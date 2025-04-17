package main

func main() {
	LOG.Info("start configure...")
	Configure()
	LOG.Info("configure ok")

	errCh := make(chan SMTPError, 1)
	finishCh := make(chan *SMTP, 1)

	smtp := NewSMTP(
		CONFIG.SMTP.Server,
		CONFIG.SMTP.SSL,
		CONFIG.SMTP.User,
		CONFIG.SMTP.Passwd,
		CONFIG.SMTP.Sender,
		CONFIG.Name,
		3,
		nil,
		errCh,
		finishCh,
	)
	defer smtp.Close()

	smtp.SendMail(
		CONFIG.SMTPAdminReceivers[0],
		"测试邮件",
		"Content-Type: text/plain; charset=UTF8;\r\n\r\n这是一封测试邮件",
	)

	select {
	case err := <-errCh:
		LOG.Error(err.progress, " ", err.err)
	case <-finishCh:
		LOG.Info("send mail success")
	}
}
