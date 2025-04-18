package main

import (
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"net/http"
	"os"
	"time"
)

type Config struct {
	Name   string   `json:"name"`
	IPAPIs []string `json:"ip_apis"`
	SMTP   struct {
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
		DomainName      string `json:"domain_name"`
		RR              string `json:"rr"`
		Type            string `json:"type"`
	} `json:"aliyun"`
	Cron string `json:"cron"`
}

var log = InitLog()
var config = Config{
	IPAPIs:             make([]string, 0),
	SMTPAdminReceivers: make([]string, 0),
	SMTPUserReceivers:  make([]string, 0),
}
var tlsConfig = &tls.Config{
	ClientAuth: tls.RequireAndVerifyClientCert,
	MinVersion: tls.VersionTLS13,
}
var httpClient = &http.Client{
	Timeout: 5 * time.Second,
}

var aliyunApiUrl = "https://alidns.aliyuncs.com"
var aliyunApiVersion = "2015-01-09"

func Configure() {
	fileData, err := os.ReadFile("data/config.json")
	if err != nil {
		fileData, err := json.MarshalIndent(config, "", "  ")
		if err != nil {
			panic(err)
		}
		err = os.WriteFile("data/config.json", fileData, 0600)
		if err != nil {
			panic(err)
		}
		log.Info("Please configure file \"data/config.json\" and then start AliyunDDNS")
		os.Exit(0)
	}
	err = json.Unmarshal(fileData, &config)
	if err != nil {
		panic(err)
	}

	systemPool, err := x509.SystemCertPool()
	if err != nil || systemPool == nil {
		systemPool = nil
		tlsConfig.ClientAuth = tls.NoClientCert // 不需要对端证书
	}
	tlsConfig.RootCAs = systemPool
}
