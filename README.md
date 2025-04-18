# AliyunDDNS
阿里云DDNS

## 描述
AliyunDDNS  
顾名思义, 是一个用于动态更新阿里云域名解析的小工具  
它通过你自定义的获取公网IP的API, 查询到你的本机IP  
然后通过阿里云的DNS服务, 将你的域名解析到你的本机IP  
中间如果产生任何错误, 则会记录错误日志, 并且通过邮件发送错误日志  
当然如果更新成功也会通过邮件发送信息

## 构建运行
### 准备材料
* 阿里云
   * 账户
   * 域名
   * RAM访问控制用户(添加AliyunDNSFullAccess权限)
* 发送者邮箱(启用SMTP服务并获取SMTP密钥)
* 接收者邮箱
* go

**如上材料缺一不可**

### 开始构建
1. 克隆仓库:
   ```
   git clone https://sxjc1.staticplant.top:7443/Zightch/AliyunDDNS.git
   cd AliyunDDNS
   ```

2. 构建项目:
   ```
   go mod tidy
   go build
   ```

### 使用
2. 首次运行AliyunDDNS, 然后关闭:
   ```bash
   ./AliyunDDNS
   ```
3. 配置`data`文件夹下的`config.json`文件
   ```json
   {
     "name": "",
     "ip_apis": [],
     "smtp": {
       "server": "",
       "ssl": false,
       "user": "",
       "passwd": "",
       "sender": ""
     },
     "smtp_admin_receivers": [],
     "smtp_user_receivers": [],
     "aliyun": {
       "access_key_id": "",
       "access_key_secret": "",
       "domain": "",
       "rr": "",
       "type": ""
     },
     "cron": ""
   }
   ```
4. 再次运行AliyunDDNS:
   ```bash
   ./AliyunDDNS
   ```
5. (可选)加入开机自启

## 许可证
[MIT License](LICENSE)

## 联系作者
* 邮箱: zightch@163.com
* QQ: 2166825850
