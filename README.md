# CryMail - 安全邮件客户端

一个基于C语言开发的安全邮件客户端，支持数字签名功能，确保邮件的认证性和完整性。

## 功能特点

- 使用RSA算法进行数字签名
- 使用SHA-256进行消息摘要
- 支持SMTP发送邮件
- 支持SSL/TLS加密传输
- 命令行界面操作
- 配置文件保存设置

## 编译依赖

- OpenSSL开发库
- libcurl开发库

Ubuntu/Debian系统安装依赖：
```bash
sudo apt-get install libssl-dev libcurl4-openssl-dev
```

CentOS/RHEL系统安装依赖：
```bash
sudo yum install openssl-devel libcurl-devel
```

## 编译方法

```bash
make clean
make
```

## 使用方法

1. 生成密钥对：
```bash
./crymail -g
```

2. 配置邮件服务器：
```bash
./crymail -c
```

3. 发送签名邮件：
```bash
./crymail -m <收件人> <主题> <消息>
```

4. 图形界面模式：
```bash
./crymail
```

5. 选择解析特定邮件：
   - 在接收邮件页面中输入要解析的邮件编号。

## 支持的邮件服务器

### 发送邮件
- 163邮箱: smtp.163.com:25
- 126邮箱: smtp.126.com:465

### 接收邮件
- 163邮箱: pop.163.com:995
- 126邮箱: pop.126.com:110

## 注意事项

1. 使用前需要先生成RSA密钥对
2. 使用Gmail需要开启"应用专用密码"
3. 使用QQ邮箱需要开启"POP3/SMTP服务"并使用授权码
4. 使用163邮箱需要开启"SMTP服务"并使用授权密码
5. 使用126邮箱需要开启"SMTP服务"并使用授权码

## 推荐配置
```
smtp_server=smtp.126.com
username=xxxx@126.com
password=xxxxx
port=465
use_ssl=1
pop3_server=pop.126.com
pop3_port=110
pop3_use_ssl=0
```

## 许可证

MIT License 