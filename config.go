package main

import (
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"os"
)

type Config struct {
	Name         string   `json:"name"`
	GetIPServers []string `json:"get_ip_servers"`
	SMTP         struct {
		Server string `json:"server"`
		SSL    bool   `json:"ssl"`
		User   string `json:"user"`
		Passwd string `json:"passwd"`
		Sender string `json:"sender"`
	} `json:"smtp"`
	SMTPAdminReceivers []string `json:"smtp_admin_receivers"`
	SMTPUserReceivers  []string `json:"smtp_user_receivers"`
	Aliyun             struct {
		AccessKeyId     string `json:"access_key_id"`
		AccessKeySecret string `json:"access_key_secret"`
		Domain          string `json:"domain"`
		RR              string `json:"rr"`
		Type            string `json:"type"`
	} `json:"aliyun"`
	Cron string `json:"cron"`
}

var LOG = InitLog()
var CONFIG = Config{
	GetIPServers:       make([]string, 0),
	SMTPAdminReceivers: make([]string, 0),
	SMTPUserReceivers:  make([]string, 0),
}
var TlsConfig = &tls.Config{
	ClientAuth: tls.RequireAndVerifyClientCert,
	MinVersion: tls.VersionTLS13,
}

func Configure() {
	fileData, err := os.ReadFile("data/config.json")
	if err != nil {
		fileData, err := json.MarshalIndent(CONFIG, "", "  ")
		if err != nil {
			panic(err)
		}
		err = os.WriteFile("data/config.json", fileData, 0600)
		if err != nil {
			panic(err)
		}
		LOG.Info("Please configure file \"data/config.json\" and then start AliyunDDNS")
		os.Exit(0)
	}
	err = json.Unmarshal(fileData, &CONFIG)
	if err != nil {
		panic(err)
	}

	systemPool, err := x509.SystemCertPool()
	if err != nil || systemPool == nil {
		systemPool = nil
		TlsConfig.ClientAuth = tls.NoClientCert // 不需要对端证书
	}
	TlsConfig.RootCAs = systemPool
}
