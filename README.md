# AliyunDDNS
阿里云DDNS  
[国内镜像](https://sxjc1.staticplant.top:4006/Zightch/AliyunDDNS)

## 描述
AliyunDDNS  
顾名思义, 是一个用于更新阿里云域名解析的小工具  
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
* C++编译器(msvc, g++, clang等)
* CMake
* Qt
* 构建工具(make, ninja, msvc等)

**如上材料缺一不可**

### 开始构建
1. 克隆仓库:
   ```bash
   git clone https://github.com/Zightch/AliyunDDNS.git
   cd AliyunDDNS
   ```

2. 构建项目:
   ```bash
   mkdir build
   cd build
   cmake ..
   make # (或替换为你的其他构建攻击指令)
   ```

### 使用
1. 将configs文件夹拷贝到可执行文件目录
   ```bash
   cp -r ../configs ./
   ```
2. 首次运行AliyunDDNS, 然后关闭:
   ```bash
   ./AliyunDDNS
   ```
3. 配置configs文件夹下的`AliyunDDNS.ini`文件
   ```ini
   [DDNS]
   name= # 当前服务名称, 可根据此名称来区分是哪个DDNS服务
   get-IP-servers=./configs/get-IP-servers.json # 获取公网IP的API列表, 可写多个

   [SMTP]
   server= # SMTP服务器地址
   SSL= # 是否启用SSL
   user= # SMTP用户名
   passwd= # SMTP密码
   sender= # 发送者邮箱
   to=./configs/receiver-emails.json # 接收者邮箱列表, 可写多个

   [Aliyun]
   AccessKey-ID= # 阿里云RAM访问控制用户 AccessKey ID
   AccessKey-Secret= # 阿里云RAM访问控制用户 AccessKey Secret
   DomainName= # 域名
   RR= # 解析记录
   Type= # 解析记录类型, 目前仅支持A和AAAA

   # 下面的配置一般情况不用动他, 除非你有特殊需求
   [MailMsg]
   start=./configs/start.html
   notify0=./configs/notify0.html
   notify1=./configs/notify1.html
   alarms0=./configs/alarms0.html
   alarms1=./configs/alarms1.html
   alarms2=./configs/alarms2.html
   alarms3=./configs/alarms3.html
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
