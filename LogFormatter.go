package main

import (
	"fmt"
	"github.com/sirupsen/logrus"
	"os"
)

type LogFormatter struct{}

func (f *LogFormatter) Format(entry *logrus.Entry) ([]byte, error) {
	timeStr := entry.Time.Format("01-02 15:04:05") // 获取时间
	level := entry.Level.String()                  // 获取日志等级
	if entry.Caller != nil {                       // 如果程序包含调试信息
		caller := entry.Caller
		logString := fmt.Sprintf("[%s][%s][%s][%d] %s\n", timeStr, level, caller.Function, caller.Line, entry.Message)
		return []byte(logString), nil
	} else { // 如果程序不包含调试信息
		message := entry.Message
		defaultLogString := fmt.Sprintf("[%s][%s] %s\n", timeStr, level, message)
		return []byte(defaultLogString), nil
	}
}

func InitLog() *logrus.Logger {
	logger := logrus.New()
	logger.SetReportCaller(true)
	logger.Formatter = &LogFormatter{}
	logger.Out = os.Stderr
	logger.SetLevel(logrus.TraceLevel)
	return logger
}
