package main

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"time"
)

func getIP() {
	type WebReply struct {
		server string
		body   []byte
		err    string
	}
	ch := make(chan WebReply)
	webReplys := make([]WebReply, 0)
	for _, server := range config.IPAPIs {
		go func() {
			req, err := http.NewRequest("GET", server, nil)
			if err != nil {
				ch <- WebReply{server, nil, fmt.Sprint(err)}
				return
			}
			resp, err := httpClient.Do(req)
			if err != nil {
				ch <- WebReply{server, nil, fmt.Sprint(err)}
				return
			}
			defer resp.Body.Close()
			body, err := io.ReadAll(resp.Body)
			if err != nil {
				ch <- WebReply{server, nil, fmt.Sprint(err)}
				return
			}
			ch <- WebReply{server, body, ""}
		}()
	}
	for len(webReplys) < len(config.IPAPIs) {
		webReply := <-ch
		webReplys = append(webReplys, webReply)
	}
	for _, webReply := range webReplys {
		if webReply.err != "" {
			failMap[webReply.server] = webReply.err
			log.Warning(webReply.err)
			continue
		}
		str := string(webReply.body)
		if IsIPv4(str) {
			ipv4Map[webReply.server] = str
			log.Info(webReply.server, " reply IP is: ", str)
		} else if IsIPv6(str) {
			ipv6Map[webReply.server] = str
			log.Info(webReply.server, " reply IP is: ", str)
		} else {
			type Body struct {
				Ip string `json:"ip"`
			}
			var body Body
			if err := json.Unmarshal(webReply.body, &body); err != nil {
				err := fmt.Sprint(webReply.server, " reply data not is json")
				failMap[webReply.server] = err
				log.Warning(err)
				continue
			}
			if IsIPv4(body.Ip) {
				ipv4Map[webReply.server] = body.Ip
				log.Info(webReply.server, " reply IP is: ", body.Ip)
			} else if IsIPv6(body.Ip) {
				ipv6Map[webReply.server] = body.Ip
				log.Info(webReply.server, " reply IP is: ", body.Ip)
			} else {
				err := fmt.Sprint(webReply.server, " in reply data not found ip")
				failMap[webReply.server] = err
				log.Warning(err)
			}
		}
	}
}

func describeDomainRecords() (string, string, error) {
	query := url.Values{}
	query.Add("Action", "DescribeDomainRecords")                            // 操作接口名
	query.Add("DomainName", config.Aliyun.DomainName)                       // 域名
	query.Add("RRKeyWord", config.Aliyun.RR)                                // 主机记录的关键字
	query.Add("Format", "JSON")                                             // 返回数据的格式
	query.Add("Version", AliyunApiVersion)                                  // API版本号
	query.Add("AccessKeyId", config.Aliyun.AccessKeyId)                     // AccessKeyId
	query.Add("SignatureMethod", "HMAC-SHA1")                               // 签名方式
	query.Add("Timestamp", time.Now().UTC().Format("2006-01-02T15:04:05Z")) // 请求的时间戳
	query.Add("SignatureVersion", "1.0")                                    // 签名算法版本
	query.Add("SignatureNonce", fmt.Sprintf("%x", time.Now().UnixNano()))   // 唯一随机数

	signature := AliyunGenSign(query, config.Aliyun.AccessKeySecret)
	query.Add("Signature", signature)

	requestUrl := fmt.Sprintf("%s/?%s", AliyunApiUrl, query.Encode())
	req, err := http.NewRequest("GET", requestUrl, nil)
	if err != nil {
		return "", "", err
	}
	resp, err := httpClient.Do(req)
	if err != nil {
		return "", "", err
	}
	defer resp.Body.Close()
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return "", "", err
	}
	var response struct {
		Code          string `json:"Code"`
		Message       string `json:"Message"`
		DomainRecords struct {
			Record []struct {
				RecordId string `json:"RecordId"`
				Value    string `json:"Value"`
			} `json:"Record"`
		} `json:"DomainRecords"`
	}
	if err := json.Unmarshal(body, &response); err != nil {
		return "", "", err
	}
	if response.Code != "" {
		return "", "", fmt.Errorf("aliyun api error. code: %s, msg: %s", response.Code, response.Message)
	}
	if len(response.DomainRecords.Record) == 0 {
		return "", "", fmt.Errorf("not found record")
	}
	return response.DomainRecords.Record[0].Value, response.DomainRecords.Record[0].RecordId, nil
}

func updateDomainRecord(recordId, newIP string) (string, error) {
	query := url.Values{}
	query.Add("Action", "UpdateDomainRecord")                               // 操作接口名
	query.Add("RecordId", recordId)                                         // 解析记录的ID
	query.Add("RR", config.Aliyun.RR)                                       // 主机记录
	query.Add("Type", config.Aliyun.Type)                                   // 记录类型
	query.Add("Value", newIP)                                               // 记录值
	query.Add("Format", "JSON")                                             // 返回数据的格式
	query.Add("Version", AliyunApiVersion)                                  // API版本号
	query.Add("AccessKeyId", config.Aliyun.AccessKeyId)                     // AccessKeyId
	query.Add("SignatureMethod", "HMAC-SHA1")                               // 签名方式
	query.Add("Timestamp", time.Now().UTC().Format("2006-01-02T15:04:05Z")) // 请求的时间戳
	query.Add("SignatureVersion", "1.0")                                    // 签名算法版本
	query.Add("SignatureNonce", fmt.Sprintf("%x", time.Now().UnixNano()))   // 唯一随机数

	signature := AliyunGenSign(query, config.Aliyun.AccessKeySecret)
	query.Add("Signature", signature)

	requestUrl := fmt.Sprintf("%s/?%s", AliyunApiUrl, query.Encode())

	req, err := http.NewRequest("GET", requestUrl, nil)
	if err != nil {
		return "", err
	}
	resp, err := httpClient.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}
	var response struct {
		Code     string `json:"Code"`
		Message  string `json:"Message"`
		RecordId string `json:"RecordId"`
	}
	if err := json.Unmarshal(body, &response); err != nil {
		return "", err
	}
	if response.Code != "" {
		return "", fmt.Errorf("aliyun api error. code: %s, msg: %s", response.Code, response.Message)
	}
	return response.RecordId, nil
}
