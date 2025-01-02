# AliyunDDNS
阿里云DDNS  
[国内镜像](https://sxjc1.staticplant.top:4006/Zightch/AliyunDDNS)

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
   make # 或替换为你的其他构建工具指令
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

## 其他细节
* 程序启动, 查询无需更新, 已经更新完成, 查询失败, 更新失败  
  如上操作都会发邮件到你的邮箱
* 每过整点十分钟会进行一次域名IP检查  
  需要修改的话可以自行修改文件[DDNS_GetIP.cpp](DDNS/DDNS_GetIP.cpp)  
  文件第35行
  ```c++
  QDateTime baseDateTime = QDateTime::currentDateTime(); // 创建基准时间点
  int msec; // 距离基准时间点的毫秒数
  {
      auto currDateTime = QDateTime::currentDateTime();
      // 提取当前时间和日期
      QTime currentTime = baseDateTime.time();
      int currentMinutes = currentTime.minute();

      // 计算距离下一个10分钟的分钟差
      int minutesToNext10 = (10 - currentMinutes % 10) % 10;
      if (minutesToNext10 == 0) {
          minutesToNext10 = 10; // 如果正好在10分钟的边界上，则跳到下一个10分钟
      }

      // 构建下一个10分钟的时间点，使用addSecs()自动生成正确的时间点
      baseDateTime = baseDateTime.addSecs(minutesToNext10 * 60 - currentTime.second() - currentTime.msec() / 1000);
      msec = int(baseDateTime.toMSecsSinceEpoch() - currDateTime.toMSecsSinceEpoch());
   }
  ```
  修改后重新编译运行
* 如果每次都失败, 那么每过`10分钟`你的邮箱就会有一条邮件, 这显然不合适  
  所以`alarms`类邮件如果每次都出错, 则`一个小时`只会发一次, 如果需要修改  
  可以在全局项目里搜`alarmsLastSendDateTime`相关字眼
* notify0(即无需更新通知)邮件只会在程序启动时和每月的1日早上8点整发  
  其他时间段全部静默  
  如需修改的话, 自行修改文件[DDNS_updateDNS.cpp](DDNS/DDNS_updateDNS.cpp)  
  文件第225行
  ```c++
  auto currDate = QDateTime::currentDateTime();
  auto currHour = currDate.toString("hh").toInt();
  auto currMin = currDate.toString("mm").toInt();
  auto currDay = currDate.toString("dd").toInt();

  if (isStart || (currHour == 8 && currDay == 1 && currMin < 10)) {
      currMailGroup = "notify0";
      ...
  ```
* notify1(即更新成功通知)邮件会在每次更新成功时发送, 这个一般情况应该是正确的需求  
  如果你非要修改的话  
  自行修改文件[DDNS_updateDNS.cpp](DDNS/DDNS_updateDNS.cpp)  
  文件第298行
  ```c++
  currMailGroup = "notify1";
  ...
  ```

> 之所以让你们自行修改有关定时任务的代码, 是因为定时任务种类太多  
> 包括但不限于
> * 10分钟整点还是10分钟之后
> * 每天定时几点还是会有若干的定时点
> * 每个星期几还是每月的第几天, 还是每星期或每月都会有若干定时点
> * 邮件的发送定时以及逻辑等(同上)
> 
> 一个小工具内置完整的定时任务逻辑引擎显然不合适, 所以只能由你们自己来决定  
