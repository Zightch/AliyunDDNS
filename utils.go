package main

import (
	"crypto/hmac"
	"crypto/sha1"
	"encoding/base64"
	"net"
	"net/url"
	"sort"
	"strings"
)

func AliyunGenSign(query url.Values, accessKeySecret string) string {
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

	// 使用HMAC-SHA1算法计算签名, 密钥为AccessKeySecret加上一个&
	key := accessKeySecret + "&"
	h := hmac.New(sha1.New, []byte(key))
	h.Write([]byte(stringToSign))
	signature := base64.StdEncoding.EncodeToString(h.Sum(nil))

	return signature
}

func IsIPv4(ipStr string) bool {
	ip := net.ParseIP(ipStr)
	return ip != nil && ip.To4() != nil
}

func IsIPv6(ipStr string) bool {
	ip := net.ParseIP(ipStr)
	return ip != nil && ip.To16() != nil && ip.To4() == nil
}

func GetMapKeys[K comparable, V any](m map[K]V) []K {
	keys := make([]K, 0, len(m)) // 创建一个切片, 长度为 map 的大小
	for key := range m {
		keys = append(keys, key) // 将每个键添加到切片中
	}
	return keys
}
