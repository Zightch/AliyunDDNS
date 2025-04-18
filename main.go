package main

import (
	"github.com/robfig/cron/v3"
	lua "github.com/yuin/gopher-lua"
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
		panic("aliyun.type must is A or AAAA")
	}
	c := cron.New()
	_, err := c.AddFunc(config.Cron, updateDNS)
	if err != nil {
		panic(err)
	}

	log.Info("configure ok")

	lState = lua.NewState()
	defer lState.Close()
	err = lState.DoFile("data/mail_strategy.lua")
	if err != nil {
		panic(err)
	}

	updateDNS()
	first = false
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
		Alarms2()
		log.Warning("all api failed")
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
		Alarms3()
		log.Warning("not found target type ip")
		return
	}
	if !equal {
		Alarms0()
		log.Warning("ip not all equal")
		return
	}

	value, recordId, err := DescribeDomainRecords()
	if err != nil {
		Alarms1(err)
		log.Warning(err)
		return
	}
	log.Info("DescribeDomainRecords ok, value: ", value, " RecordId: ", recordId)

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
		Notify0(value)
		log.Info("not need update")
		return
	}
	newRecordId, err := UpdateDomainRecord(recordId, newIP)
	if err != nil {
		Alarms1(err)
		log.Warning(err)
		return
	}
	log.Info("UpdateDomainRecord ok, new RecordId: ", newRecordId)
	Notify1(value, newIP)
	log.Info("update ok")
}
