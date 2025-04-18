package main

import (
	"encoding/json"
	"fmt"
	"github.com/robfig/cron/v3"
	"io"
	"net/http"
	"time"
)

func main() {
	log.Info("start configure...")

	Configure()
	c := cron.New()
	_, err := c.AddFunc(config.Cron, updateDNS)
	if err != nil {
		panic(err)
	}

	log.Info("configure ok")

	updateDNS()
	c.Start()
	select {}
}

func updateDNS() {
	log.Info("start updating")
	ipv4 := make(map[string]string)
	ipv6 := make(map[string]string)
	fail := make(map[string]string)
	getIP(ipv4, ipv6, fail)

}

func getIP(ipv4, ipv6, fail map[string]string) {
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
			var client = &http.Client{
				Timeout: 5 * time.Second,
			}
			resp, err := client.Do(req)
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
			fail[webReply.server] = webReply.err
			log.Warning(webReply.err)
			continue
		}
		str := string(webReply.body)
		if IsIPv4(str) {
			ipv4[webReply.server] = str
			log.Info(webReply.server, " reply IP is: ", str)
		} else if IsIPv6(str) {
			ipv6[webReply.server] = str
			log.Info(webReply.server, " reply IP is: ", str)
		} else {
			type Body struct {
				Ip string `json:"ip"`
			}
			var body Body
			if err := json.Unmarshal(webReply.body, &body); err != nil {
				err := fmt.Sprint(webReply.server, " 返回数据内容不是json")
				fail[webReply.server] = err
				log.Warning(err)
				continue
			}
			if IsIPv4(body.Ip) {
				ipv4[webReply.server] = body.Ip
				log.Info(webReply.server, " reply IP is: ", body.Ip)
			} else if IsIPv6(body.Ip) {
				ipv6[webReply.server] = body.Ip
				log.Info(webReply.server, " reply IP is: ", body.Ip)
			} else {
				err := fmt.Sprint(webReply.server, " 返回数据内容没有ip信息")
				fail[webReply.server] = err
				log.Warning(err)
			}
		}
	}
}

func describeDomainRecords() {

}
