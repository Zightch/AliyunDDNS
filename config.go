package main

import (
	"encoding/json"
	"gopkg.in/ini.v1"
	"os"
)

const configFilePath = "AliyunDDNS.ini"

type DDNS struct {
	AccessKeyID     string
	AccessKeySecret string
	DomainName      string
	RR              string
	Type            string
	getIPServers    []string
}

func Config() *DDNS {
	cfg, err := ini.Load(configFilePath)
	if err != nil {
		log.Fatal("读取配置文件失败: ", err)
	}

	ddnsSection := cfg.Section("DDNS")
	aliyunSection := cfg.Section("Aliyun")
	var ddns = DDNS{
		AccessKeyID:     aliyunSection.Key("AccessKey-ID").String(),
		AccessKeySecret: aliyunSection.Key("AccessKey-Secret").String(),
		DomainName:      aliyunSection.Key("DomainName").String(),
		RR:              aliyunSection.Key("RR").String(),
		Type:            aliyunSection.Key("Type").String(),
	}
	getIPServers := ddnsSection.Key("get-IP-servers").String()
	jsonData, err := os.ReadFile(getIPServers)
	if err != nil {
		log.Fatalf("无法读取%s: %v", getIPServers, err)
	}

	if err := json.Unmarshal(jsonData, &ddns.getIPServers); err != nil {
		log.Fatalf("%s格式有误: %v", getIPServers, err)
	}
	return &ddns
}
