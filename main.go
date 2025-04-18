package main

import (
	"github.com/robfig/cron/v3"
	"sync"
)

var (
	ipv4Map map[string]string
	ipv6Map map[string]string
	failMap map[string]string
	mu      sync.Mutex
)

func main() {
	log.Info("start configure...")

	Configure()
	if config.Aliyun.Type != "A" && config.Aliyun.Type != "AAAA" {
		panic("aliyun.type must be A or AAAA")
	}
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
	mu.Lock() // 防止多个协程同时更新
	defer mu.Unlock()

	log.Info("start update...")

	ipv4Map = make(map[string]string)
	ipv6Map = make(map[string]string)
	failMap = make(map[string]string)

	GetIP()

	if len(failMap) == len(config.IPAPIs) { // 所有api全部失败
		log.Warning("all api failed")
		Alarms2()
		return
	}

	equal := true  // 标记所有ip相等
	empty := false // 标记目标类型ip为空
	if config.Aliyun.Type == "A" {
		if len(ipv4Map) == 0 {
			empty = true
		} else {
			keys := GetMapKeys(ipv4Map)
			for i := 1; i < len(keys); i++ {
				if ipv4Map[keys[i-1]] != ipv4Map[keys[i]] {
					equal = false
					break
				}
			}
		}
	} else if config.Aliyun.Type == "AAAA" {
		if len(ipv6Map) == 0 {
			empty = true
		} else {
			keys := GetMapKeys(ipv6Map)
			for i := 1; i < len(keys); i++ {
				if ipv6Map[keys[i-1]] != ipv6Map[keys[i]] {
					equal = false
					break
				}
			}
		}
	}

	if empty {
		log.Warning("not found target type ip")
		Alarms3()
		return
	}
	if !equal {
		log.Warning("ip not all equal")
		Alarms0()
		return
	}

	value, recodId, err := DescribeDomainRecords()
	if err != nil {
		log.Warning(err)
		Alarms1(err)
		return
	}
	log.Info("DescribeDomainRecords ok, value: ", value, " recodId: ", recodId)

	var newIP string
	if config.Aliyun.Type == "A" {
		keys := GetMapKeys(ipv4Map)
		newIP = ipv4Map[keys[0]]
	} else if config.Aliyun.Type == "AAAA" {
		keys := GetMapKeys(ipv6Map)
		newIP = ipv6Map[keys[0]]
	}
	equal = newIP == value

	if equal {
		log.Info("not need update")
		Notify0()
		return
	}
	newRecordId, err := UpdateDomainRecord(recodId, newIP)
	if err != nil {
		log.Warning(err)
		Alarms1(err)
		return
	}
	log.Info("UpdateDomainRecord ok, new recordId: ", newRecordId)
	Notify1(value, newIP)
	log.Info("update ok")
}
