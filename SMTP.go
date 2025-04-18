package main

import (
	"crypto/tls" // 导入TLS加密库
	"encoding/base64"
	"encoding/hex"
	"fmt" // 导入格式化输入输出库
	"net" // 导入网络库
	"regexp"
	"strconv"
	"strings" // 导入字符串处理库
	"sync"    // 导入同步库
	"time"    // 导入时间库
)

// expectRetCode 期望服务器返回的代码
var expectRetCode = []int{0, 220, 250, 334, 334, 235, 250, 250, 354, 250, 221}
var retMsgRe0 = regexp.MustCompile(`^[1-5]\d{2}\s.+$`)
var retMsgRe1 = regexp.MustCompile(`^[1-5]\d{2}-.+$`)

const TOTAL_STEPS = 10 // 总步数

// SMTPError 表示SMTP错误结构体
type SMTPError struct {
	smtp     *SMTP
	progress int   // 错误发生时的进度编号
	err      error // 错误信息
}

type SMTPProgress struct {
	smtp     *SMTP
	progress int // 进度编号
}

// SMTP 表示SMTP客户端结构体
type SMTP struct {
	server     string // SMTP服务器地址
	ssl        bool   // 是否使用SSL/TLS加密
	user       string // 用户名
	passwd     string // 密码
	senderMail string // 发件人邮箱
	senderName string // 发件人名称
	receiver   string // 收件人邮箱
	subject    string // 邮件主题
	body       string // 邮件正文

	mu          sync.Mutex // conn互斥锁
	conn        net.Conn   // 当前连接对象
	progressNum int        // 当前进度编号
	waitTime    int        // 超时时间(秒)
	activeClose bool       // 主动关闭

	progressCh chan SMTPProgress // 进度通知通道
	errCh      chan SMTPError    // 错误通知通道
	finishCh   chan *SMTP        // 完成通知通道
}

// NewSMTP 创建一个新的SMTP客户端实例
func NewSMTP(
	server string,
	ssl bool,
	user,
	passwd,
	sendMail,
	sendName string,
	waitTime int,
	progressCh chan SMTPProgress,
	errCh chan SMTPError,
	finishCh chan *SMTP,
) *SMTP {
	return &SMTP{
		server:      server,     // 设置SMTP服务器地址
		ssl:         ssl,        // 设置是否启用SSL/TLS
		user:        user,       // 设置用户名
		passwd:      passwd,     // 设置密码
		senderMail:  sendMail,   // 设置发件人邮箱
		senderName:  sendName,   // 设置发件人名称
		waitTime:    waitTime,   // 设置连接超时时间
		progressCh:  progressCh, // 设置进度通知通道
		errCh:       errCh,      // 设置错误通知通道
		finishCh:    finishCh,   // 设置完成通知通道
		progressNum: 0,          // 初始化进度编号
		conn:        nil,        // 初始化连接对象
		activeClose: false,      // 初始化是否主动关闭
	}
}

// SendMail 发送邮件
func (s *SMTP) SendMail(receiver, subject, body string) string {
	s.mu.Lock()
	if s.conn != nil { // 如果当前连接不为空, 则表示正在发送邮件
		s.mu.Unlock()
		return "current SMTP sending mail, please wait sent" // 直接返回, 避免重复发送
	}

	s.receiver = receiver                                      // 设置收件人邮箱
	s.subject = subject                                        // 设置邮件主题
	body = strings.ReplaceAll(body, "\n", "\r\n")              // 替换换行符为标准格式
	body = strings.ReplaceAll(body, "\r\r\n", "\r\n")          // 去除多余的回车符
	body = strings.ReplaceAll(body, "\r", "\r\n")              // 替换回车符为标准格式
	body = strings.ReplaceAll(body, "\r\n\n", "\r\n")          // 去除多余的换行符
	body = strings.ReplaceAll(body, "\r\n.\r\n", "\r\n..\r\n") // 处理以`.`开头的行
	s.body = body                                              // 设置邮件正文

	s.activeClose = false
	s.progressNum = 0

	var connectMsg = ""   // 初始化连接消息
	var wg sync.WaitGroup // 初始化等待组
	wg.Add(1)             // 添加一个等待任务
	go func() {           // 创建连接的goroutine
		dialer := &net.Dialer{
			Timeout: time.Second * time.Duration(s.waitTime), // 设置连接超时时间
		}
		var conn net.Conn // 定义连接对象
		var err error     // 定义错误对象
		if s.ssl {        // 如果启用SSL/TLS
			conn, err = tls.DialWithDialer(dialer, "tcp", s.server, tlsConfig) // 使用TLS建立连接
		} else { // 否则
			conn, err = dialer.Dial("tcp", s.server) // 使用普通TCP建立连接
		}
		if err != nil { // 如果连接失败
			s.conn = nil                                                              // 重置连接对象
			connectMsg = fmt.Sprintf("connect to server %s error: %v", s.server, err) // 记录错误消息
		} else { // 如果连接成功
			s.conn = conn   // 设置连接对象
			connectMsg = "" // 清空错误消息
		}
		wg.Done() // 完成等待任务
	}()
	go func() { // 连接后处理的goroutine
		wg.Wait()             // 等待连接任务完成
		if connectMsg != "" { // 如果存在连接错误
			if s.errCh != nil { // 如果存在错误通知通道
				s.errCh <- SMTPError{s, s.progressNum, fmt.Errorf(connectMsg)} // 通知错误
			}
			s.mu.Unlock()
			return // 直接返回
		}

		s.progressNum++

		s.main() // 进入主逻辑
	}()
	return ""
}

func (s *SMTP) Close() {
	s.mu.Lock()
	s.activeClose = true
	if s.conn != nil { // 如果连接不为空
		s.conn.Close()
	}
	s.conn = nil
	s.mu.Unlock()
	s.progressNum = 0
}

func (s *SMTP) main() {
	waitTime := time.Second * time.Duration(s.waitTime)
	timer := time.NewTimer(waitTime)
	if !timer.Stop() { // 初始化
		select {
		case <-timer.C:
		default:
		}
	}

	dataCh := make(chan []byte)
	buff := make([]byte, 0)
	var err *error
	s.mu.Unlock() // 对应SendMail中的Lock()

	for {
		var data []byte

		timer.Reset(waitTime) // 重置定时器

		go s.smtpConnReader(dataCh, &err, &buff) // 读取连接数据

		select { // 等待回复或超时
		case <-timer.C:
			s.error(fmt.Errorf("server response timeout"))
			<-dataCh // 确保smtpConnReader退出, 防止goroutine泄露
			return
		case d := <-dataCh:
			data = d
			break
		}
		// 能执行到这里说明smtpConnReader退出, 不会goroutine泄露

		// 已回复
		timer.Stop() // 停止定时器
		select {     // 清除timer.C
		case <-timer.C:
		default:
		}

		if err != nil {
			s.error(*err)
			return
		}
		// 无错误
		str := string(data)
		retMsgMatch := retMsgRe0.MatchString(str) // 匹配"数字空格内容"
		if retMsgMatch {                          // 能匹配上
			retCode, _ := strconv.Atoi(str[:3])          // 提取数字
			if retCode != expectRetCode[s.progressNum] { // 返回码不匹配
				s.error(fmt.Errorf(str))
				return
			}
		} else { // 匹配不上
			retMsgMatch = retMsgRe1.MatchString(str) // 匹配"数字-内容"
			if retMsgMatch {                         // 匹配上跳过
				continue
			}
			s.error(fmt.Errorf("invalid data: %s", hex.EncodeToString(data))) // 匹配不上报错
			return
		}
		if s.progressCh != nil {
			s.progressCh <- SMTPProgress{s, s.progressNum} // 进度通知
		}
		// 无错误
		var cmd string
		switch s.progressNum {
		case 1:
			cmd = "HELO " + s.server + "\r\n"
		case 2:
			cmd = "AUTH LOGIN\r\n"
		case 3:
			cmd = base64.StdEncoding.EncodeToString([]byte(s.user)) + "\r\n"
		case 4:
			cmd = base64.StdEncoding.EncodeToString([]byte(s.passwd)) + "\r\n"
		case 5:
			cmd = "MAIL FROM:<" + s.senderMail + ">\r\n"
		case 6:
			cmd = "RCPT TO:<" + s.receiver + ">\r\n"
		case 7:
			cmd = "DATA\r\n"
		case 8:
			cmd = "Subject:" + s.subject + "\r\n"
			cmd += "From: " + s.senderName + " <" + s.senderMail + ">\r\n"
			cmd += "To: " + s.receiver + "\r\n"
			cmd += s.body + "\r\n.\r\n"
		case 9:
			cmd = "QUIT\r\n"
		case 10:
			s.Close()
			if s.finishCh != nil { // 如果存在完成通知通道
				s.finishCh <- s // 通知完成
			}
			return
		}
		if cmd != "" {
			s.mu.Lock()
			if s.conn == nil {
				return
			}
			_, err := s.conn.Write([]byte(cmd))
			s.mu.Unlock()
			if err != nil {
				s.error(err)
				return
			}
		}
		s.progressNum++
	}
}

func (s *SMTP) error(err error) {
	s.mu.Lock()
	if s.activeClose { // 如果是主动关闭的报错, 不报错
		return
	}
	closeErr := s.conn.Close()
	s.conn = nil
	s.mu.Unlock()
	oldNum := s.progressNum - 1
	s.progressNum = 0
	if s.errCh != nil { // 如果存在错误通知通道
		if closeErr != nil { // 如果关闭连接失败
			s.errCh <- SMTPError{s, oldNum, fmt.Errorf("%v. %v", err, closeErr)} // 通知错误
		} else { // 否则
			s.errCh <- SMTPError{s, oldNum, fmt.Errorf("%v", err)} // 通知错误
		}
	}
}

func (s *SMTP) smtpConnReader(dataCh chan []byte, err **error, buff *[]byte) {
	tmp := make([]byte, 1024) // 缓冲区1024
	data := make([]byte, 0)
	var lineSize = 1 // 用于标记\r\n的位置
	for {            // 循环读取数据
		if len(*buff) >= 2 { // 当缓存长度>2时
			for lineSize < len(*buff) { // 遍历数据
				if (*buff)[lineSize-1] == '\r' && (*buff)[lineSize] == '\n' { // 如果找到了\r\n
					break // 退出循环
				}
				lineSize++
			}
			if lineSize < len(*buff) { // 如果lineSize没有找到buff的头, 说明数据包完结
				data = append(data, (*buff)[:lineSize-1]...)
				*buff = (*buff)[lineSize+1:]
				break
			}
		}

		s.mu.Lock()
		tmpConn := s.conn // 拷贝指针, 防止外部置nil时panic
		s.mu.Unlock()

		if tmpConn == nil { // 如果连接已经关闭
			if err != nil {
				e := fmt.Errorf("connection is nil")
				*err = &e
			}
			dataCh <- nil
			return
		}

		n, e := tmpConn.Read(tmp) // 外部close这里一定会返回error
		if e != nil {
			if err != nil {
				*err = &e
			}
			dataCh <- nil
			return
		}
		*buff = append(*buff, tmp[:n]...) // 只要读取到的数量个字节
	}
	if err != nil {
		*err = nil
	}
	dataCh <- data
}
