package main

import (
	lua "github.com/yuin/gopher-lua"
	"os"
	"strings"
	"time"
)

func mailStrategy(fn string) (bool, bool) { // 邮件策略
	lState.SetGlobal("first", lua.LBool(first))
	err := lState.CallByParam(
		lua.P{
			Fn:      lState.GetGlobal(fn),
			NRet:    2,
			Protect: true,
		},
	)
	if err != nil {
		log.Error(err)
		return false, false
	}
	ret1 := lState.Get(-2)
	ret2 := lState.Get(-1)
	lState.Pop(2)

	res1, ok1 := ret1.(lua.LBool)
	res2, ok2 := ret2.(lua.LBool)
	if !ok1 {
		log.Error("lua func return value error")
		return false, false
	}
	if !ok2 {
		log.Error("lua func return value error")
		return false, false
	}
	return bool(res1), bool(res2)
}

func sendMail(receivers []string, subject, body string) {
	errCh := make(chan SMTPError, len(receivers))
	finishCh := make(chan *SMTP, len(receivers))
	for _, receiver := range receivers {
		s := NewSMTP(
			config.SMTP.Server,
			config.SMTP.SSL,
			config.SMTP.User,
			config.SMTP.Passwd,
			config.SMTP.Sender,
			config.Name,
			3,
			nil,
			errCh,
			finishCh,
		)
		s.SendMail(receiver, subject, body)
	}
	for i := 0; i < len(receivers); i++ {
		select {
		case err := <-errCh:
			log.Warningf(
				"receiver: %s, progress: %d, error: %v",
				err.smtp.receiver,
				err.progress,
				err.err,
			)
		case <-finishCh:
		}
	}
}

func table() string {
	str := ""
	keys := GetMapKeys(ipv4Map)
	for _, key := range keys {
		str += "<div>" + key + "</div>\n"
		str += "<div>" + ipv4Map[key] + "</div>\n"
	}
	keys = GetMapKeys(ipv6Map)
	for _, key := range keys {
		str += "<div>" + key + "</div>\n"
		str += "<div>" + ipv6Map[key] + "</div>\n"
	}
	keys = GetMapKeys(failMap)
	for _, key := range keys {
		str += "<div>" + key + "</div>\n"
		str += "<div>" + failMap[key] + "</div>\n"
	}
	return str
}

func htmlPreHandle(htmlFilePath string) (string, error) {
	htmlB, fe := os.ReadFile(htmlFilePath)
	if fe != nil {
		return "", fe
	}
	htmlS := string(htmlB)
	htmlS = strings.ReplaceAll(htmlS, "${date}", time.Now().Format("2006-01-02 15:04:05"))
	htmlS = strings.ReplaceAll(htmlS, "${name}", config.Name)
	return htmlS, nil
}

func Alarms0() { // 返回IP不相等
	admin, user := mailStrategy("alarms0")
	if admin {
		func() {
			htmlS, he := htmlPreHandle("data/html/admin/alarms0.html")
			if he != nil {
				log.Error(he)
				return
			}
			htmlS = strings.ReplaceAll(htmlS, "${table}", table())
			sendMail(config.SMTPAdminReceivers, config.Name+" Alarms", htmlS)
		}()
	}
	if user {

	}
}

func Alarms1(err error) { // aliyun执行出错误
	admin, user := mailStrategy("alarms1")
	if admin {
		func() {
			htmlS, he := htmlPreHandle("data/html/admin/alarms1.html")
			if he != nil {
				log.Error(he)
				return
			}
			htmlS = strings.ReplaceAll(htmlS, "${log}", err.Error())
			sendMail(config.SMTPAdminReceivers, config.Name+" Alarms", htmlS)
		}()
	}
	if user {

	}
}

func Alarms2() { // 所有IP请求失败
	admin, user := mailStrategy("alarms2")
	if admin {
		func() {
			htmlS, he := htmlPreHandle("data/html/admin/alarms2.html")
			if he != nil {
				log.Error(he)
				return
			}
			htmlS = strings.ReplaceAll(htmlS, "${table}", table())
			sendMail(config.SMTPAdminReceivers, config.Name+" Alarms", htmlS)
		}()
	}
	if user {

	}
}

func Alarms3() { // 没有请求到目标的记录类型
	admin, user := mailStrategy("alarms3")
	if admin {
		func() {
			htmlS, he := htmlPreHandle("data/html/admin/alarms3.html")
			if he != nil {
				log.Error(he)
				return
			}
			htmlS = strings.ReplaceAll(htmlS, "${table}", table())
			sendMail(config.SMTPAdminReceivers, config.Name+" Alarms", htmlS)
		}()
	}
	if user {

	}
}

func Notify0(ip string) { // 无需更新
	admin, user := mailStrategy("notify0")
	if admin {
		func() {
			htmlS, he := htmlPreHandle("data/html/admin/notify0.html")
			if he != nil {
				log.Error(he)
				return
			}
			htmlS = strings.ReplaceAll(htmlS, "${table}", table())
			htmlS = strings.ReplaceAll(htmlS, "${domainName}", config.Aliyun.RR+"."+config.Aliyun.DomainName)
			htmlS = strings.ReplaceAll(htmlS, "${ip}", ip)
			sendMail(config.SMTPAdminReceivers, config.Name+" Notify", htmlS)
		}()
	}
	if user {

	}
}

func Notify1(oldIP, newIP string) { // 更新成功
	admin, user := mailStrategy("notify1")
	if admin {
		func() {
			htmlS, he := htmlPreHandle("data/html/admin/notify1.html")
			if he != nil {
				log.Error(he)
				return
			}
			htmlS = strings.ReplaceAll(htmlS, "${table}", table())
			htmlS = strings.ReplaceAll(htmlS, "${domainName}", config.Aliyun.RR+"."+config.Aliyun.DomainName)
			htmlS = strings.ReplaceAll(htmlS, "${oldIP}", oldIP)
			htmlS = strings.ReplaceAll(htmlS, "${newIP}", newIP)
			sendMail(config.SMTPAdminReceivers, config.Name+" Notify", htmlS)
		}()
	}
	if user {
		func() {
			htmlS, he := htmlPreHandle("data/html/user/notify1.html")
			if he != nil {
				log.Error(he)
				return
			}
			htmlS = strings.ReplaceAll(htmlS, "${domainName}", config.Aliyun.RR+"."+config.Aliyun.DomainName)
			htmlS = strings.ReplaceAll(htmlS, "${newIP}", newIP)
			sendMail(config.SMTPUserReceivers, config.Name+" Notify", htmlS)
		}()
	}
}
