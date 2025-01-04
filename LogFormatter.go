package main

import (
	"fmt"
	"github.com/sirupsen/logrus"
	"os"
)

type LogFormatter struct{}

func (f *LogFormatter) Format(entry *logrus.Entry) ([]byte, error) {
	timestamp := entry.Time.Format("01-02 15:04:05") // 获取时间
	threadID := os.Getpid()                          // 获取线程ID
	level := entry.Level.String()                    // 获取日志等级
	if entry.Caller != nil {                         // 如果程序包含调试信息
		caller := entry.Caller
		fileLine := fmt.Sprintf("%s:%d", caller.File, caller.Line)
		message := entry.Message
		logString := fmt.Sprintf("[%s][%d][%s][%s] %s\n", timestamp, threadID, level, fileLine, message)
		return []byte(logString), nil
	} else { // 如果程序不包含调试信息
		message := entry.Message
		defaultLogString := fmt.Sprintf("[%s][%d][%s] %s\n", timestamp, threadID, level, message)
		return []byte(defaultLogString), nil
	}
}

func InitLog() *logrus.Logger {
	logger := logrus.New()
	logger.Formatter = &LogFormatter{}
	logger.Out = os.Stderr
	logger.SetLevel(logrus.DebugLevel)
	return logger
}
