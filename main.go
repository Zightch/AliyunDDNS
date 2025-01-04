package main

import (
	"crypto/hmac"
	"crypto/sha1"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/url"
	"sort"
	"strings"
	"time"
)

func aliyunGenerateSignature(query url.Values, accessKeySecret string) string {
	// 按照参数名的字典顺序排序
	keys := make([]string, 0, len(query))
	for key := range query {
		keys = append(keys, key)
	}
	sort.Strings(keys)

	// 使用URL编码拼接参数
	var paramString strings.Builder
	for _, key := range keys {
		paramString.WriteString(url.QueryEscape(key))
		paramString.WriteString("=")
		paramString.WriteString(url.QueryEscape(query.Get(key)))
		paramString.WriteString("&")
	}
	paramStringStr := paramString.String()
	paramStringStr = paramStringStr[:len(paramStringStr)-1] // 去掉最后一个&

	// 在参数前后加上HTTP方法和URL编码的斜杠
	stringToSign := "GET&%2F&" + url.QueryEscape(paramStringStr)

	// 使用HMAC-SHA1算法计算签名，密钥为AccessKeySecret加上一个&
	key := accessKeySecret + "&"
	h := hmac.New(sha1.New, []byte(key))
	h.Write([]byte(stringToSign))
	signature := base64.StdEncoding.EncodeToString(h.Sum(nil))

	return signature
}

func isIPv4(ipStr string) bool {
	ip := net.ParseIP(ipStr)
	return ip != nil && ip.To4() != nil
}

func isIPv6(ipStr string) bool {
	ip := net.ParseIP(ipStr)
	return ip != nil && ip.To16() != nil && ip.To4() == nil
}

var log = InitLog()
var client = &http.Client{
	Timeout: 5 * time.Second,
}

const aliyunApiUrl = "https://alidns.aliyuncs.com"

const aliyunApiVersion = "2015-01-09"

func main() {
	log.Info("正在读取配置...")
	config := Config()
	if config == nil {
		log.Fatal("读取配置失败")
	}
	var ret4 []string
	var ret6 []string
	for _, server := range config.getIPServers {
		req, err := http.NewRequest("GET", server, nil)
		if err != nil {
			log.Warning("创建 ", server, " 请求错误: ", err)
			continue
		}
		resp, err := client.Do(req)
		if err != nil {
			log.Warning("发送 ", server, " 请求错误: ", err)
			continue
		}
		defer resp.Body.Close()
		body, err := io.ReadAll(resp.Body)
		if err != nil {
			log.Warning("读取 ", server, " 返回数据错误: ", err)
			continue
		}
		data := string(body)
		if isIPv4(data) {
			ret4 = append(ret4, data)
			log.Info(server, " 返回的IP地址: ", data)
		} else if isIPv6(data) {
			ret6 = append(ret6, data)
			log.Info(server, " 返回的IP地址: ", data)
		} else {
			type RetIP struct {
				Ip string `json:"ip"`
			}
			var retIP RetIP
			if err := json.Unmarshal(body, &retIP); err != nil {
				log.Info(server, " 返回数据内容不是json")
				continue
			}
			if isIPv4(retIP.Ip) {
				ret4 = append(ret4, retIP.Ip)
				log.Info(server, " 返回的IP地址: ", retIP.Ip)
			} else if isIPv6(retIP.Ip) {
				ret6 = append(ret6, retIP.Ip)
				log.Info(server, " 返回的IP地址: ", retIP.Ip)
			} else {
				log.Info(server, " 返回数据内容没有ip信息")
			}
		}
	}
	if config.Type == "A" {
		if len(ret4) == 0 {
			log.Fatal("没有找到IPv4公网地址")
		}
		for i := 0; i < len(ret4)-1; i++ {
			if ret4[i] != ret4[i+1] {
				log.Fatal("各个IP API返回的IPv4公网地址不一致, 无法更新")
			}
		}
		describeDomainRecords(config, ret4[0])
	} else if config.Type == "AAAA" {
		if len(ret6) == 0 {
			log.Fatal("没有找到IPv6公网地址")
		}
		for i := 0; i < len(ret6)-1; i++ {
			if ret6[i] != ret6[i+1] {
				log.Fatal("各个IP API返回的IPv4公网地址不一致, 无法更新")
			}
		}
		describeDomainRecords(config, ret6[0])
	} else {
		log.Fatal("配置文件Type错误, 仅支持A和AAAA")
	}
}

func describeDomainRecords(config *DDNS, ip string) {
	query := url.Values{}
	query.Add("Action", "DescribeDomainRecords")                            // 操作接口名
	query.Add("DomainName", config.DomainName)                              // 域名
	query.Add("RRKeyWord", config.RR)                                       // 主机记录的关键字
	query.Add("Format", "JSON")                                             // 返回数据的格式
	query.Add("Version", aliyunApiVersion)                                  // API版本号
	query.Add("AccessKeyId", config.AccessKeyID)                            // AccessKeyId
	query.Add("SignatureMethod", "HMAC-SHA1")                               // 签名方式
	query.Add("Timestamp", time.Now().UTC().Format("2006-01-02T15:04:05Z")) // 请求的时间戳
	query.Add("SignatureVersion", "1.0")                                    // 签名算法版本
	query.Add("SignatureNonce", fmt.Sprintf("%x", time.Now().UnixNano()))   // 唯一随机数

	signature := aliyunGenerateSignature(query, config.AccessKeySecret)
	query.Add("Signature", signature)

	requestUrl := fmt.Sprintf("%s/?%s", aliyunApiUrl, query.Encode())
	req, err := http.NewRequest("GET", requestUrl, nil)
	if err != nil {
		log.Fatal("DescribeDomainRecords创建请求失败: ", err)
	}
	resp, err := client.Do(req)
	if err != nil {
		log.Fatal("DescribeDomainRecords发送请求错误: ", err)
	}
	defer resp.Body.Close()
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		log.Fatal("DescribeDomainRecords读取返回数据失败: ", err)
	}
	type Response struct {
		Code          string `json:"Code"`
		Message       string `json:"Message"`
		DomainRecords struct {
			Record []struct {
				RecordId string `json:"RecordId"`
				Value    string `json:"Value"`
			} `json:"Record"`
		} `json:"DomainRecords"`
	}
	var response Response
	if err := json.Unmarshal(body, &response); err != nil {
		log.Fatal("DescribeDomainRecords解析返回数据失败: ", err)
	}
	if response.Code != "" {
		log.Fatalf("DescribeDomainRecords查询域名记录失败 code: %s, msg: %s", response.Code, response.Message)
	}
	if len(response.DomainRecords.Record) == 0 {
		log.Fatal("DescribeDomainRecords查询域名记录失败, 没有找到记录")
	}
	if response.DomainRecords.Record[0].Value == ip {
		log.Infof("%s.%s的记录为%s, RecordId: %s, 无需更新", config.RR, config.DomainName, ip, response.DomainRecords.Record[0].RecordId)
		return
	}
	updateDomainRecord(config, ip, response.DomainRecords.Record[0].RecordId)
}

func updateDomainRecord(config *DDNS, ip string, recordId string) {
	query := url.Values{}
	query.Add("Action", "UpdateDomainRecord")                               // 操作接口名
	query.Add("RecordId", recordId)                                         // 解析记录的ID
	query.Add("RR", config.RR)                                              // 主机记录
	query.Add("Type", config.Type)                                          // 记录类型
	query.Add("Value", ip)                                                  // 记录值
	query.Add("Format", "JSON")                                             // 返回数据的格式
	query.Add("Version", aliyunApiVersion)                                  // API版本号
	query.Add("AccessKeyId", config.AccessKeyID)                            // AccessKeyId
	query.Add("SignatureMethod", "HMAC-SHA1")                               // 签名方式
	query.Add("Timestamp", time.Now().UTC().Format("2006-01-02T15:04:05Z")) // 请求的时间戳
	query.Add("SignatureVersion", "1.0")                                    // 签名算法版本
	query.Add("SignatureNonce", fmt.Sprintf("%x", time.Now().UnixNano()))   // 唯一随机数

	signature := aliyunGenerateSignature(query, config.AccessKeySecret)
	query.Add("Signature", signature)

	requestUrl := fmt.Sprintf("%s/?%s", aliyunApiUrl, query.Encode())

	req, err := http.NewRequest("GET", requestUrl, nil)
	if err != nil {
		log.Fatal("UpdateDomainRecord创建请求失败: ", err)
	}
	resp, err := client.Do(req)
	if err != nil {
		log.Fatal("UpdateDomainRecord发送请求错误: ", err)
	}
	defer resp.Body.Close()
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		log.Fatal("UpdateDomainRecord读取返回数据失败: ", err)
	}
	type Response struct {
		Code     string `json:"Code"`
		Message  string `json:"Message"`
		RecordId string `json:"RecordId"`
	}
	var response Response
	if err := json.Unmarshal(body, &response); err != nil {
		log.Fatal("UpdateDomainRecord解析返回数据失败: ", err)
	}
	if response.Code != "" {
		log.Fatalf("UpdateDomainRecord查询域名记录失败 code: %s, msg: %s", response.Code, response.Message)
	}
	log.Infof("%s.%s更新成功: %s, 新的RecordId: %s", config.RR, config.DomainName, ip, response.RecordId)
}
