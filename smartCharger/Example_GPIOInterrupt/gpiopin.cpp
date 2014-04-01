#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>

#include "gpiopin.h"

#define MAX_BUF 64
#define SYSFS_GPIO_DIR "/sys/class/gpio"

void logError(QString methodName, int error){
	syslog(LOG_INFO, QString("gpio: %1 error: %2").arg(methodName.toLatin1().constData()).arg(error).toLatin1());
}

GpIoPin::GpIoPin(uint pinNumber, bool isOut, QObject *parent) : QObject(parent){
	_currentValue = 2;	// Make it out of range to force write if out pin
	_ioFd = -1;					// Make it invalid to force initial open
	_pinNumber = pinNumber;

	exportPin();
	setDirection(isOut);
}

GpIoPin::~GpIoPin(){
	if( 0 <= _ioFd ) close(_ioFd);
}

int GpIoPin::getValue(uint* value){
#ifdef __ARMEL__
	if( !_isOut ){
		if( 0 > _ioFd ) openFd();
		if( 0 > _ioFd ) return _ioFd;

		char ch;

		read(_ioFd, &ch, 1);

		if( ch != '0' ){
			*value = 1;
		}else{
			*value = 0;
		}

		_currentValue = *value;
	}
#endif

	return 0;
}

int GpIoPin::exportPin(){
#ifdef __ARMEL__
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if( fd < 0 ){
		logError("exportPin", fd);
		return fd;
	}

	len = qsnprintf(buf, sizeof(buf), "%d", _pinNumber);
	write(fd, buf, len);
	close(fd);
#endif

	return 0;
}

void GpIoPin::openFd(){
#ifdef __ARMEL__
	int len;
	char buf[MAX_BUF];

	len = qsnprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", _pinNumber);

	if( 0 <= _ioFd ) close(_ioFd );

	if( _isOut )
		_ioFd = open(buf, O_RDWR);
	else
		_ioFd = open(buf, O_RDONLY);

	if( 0 > _ioFd ){
		logError("openIoFd", _ioFd);
	}
#endif
}

int GpIoPin::setDirection(bool out){
#ifdef __ARMEL__
	int fd, len;
	char buf[MAX_BUF];

	len = qsnprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", _pinNumber);

	fd = open(buf, O_WRONLY);
	if( fd < 0 ){
		logError("setDirection", fd);
		return fd;
	}

	if( out ){
		write(fd, "out", 4);
	}else{
		write(fd, "in", 3);
	}

	close(fd);
#endif

	_isOut = out;

	return 0;
}

int GpIoPin::setEdge(char *edge){
#ifdef __ARMEL__
	int fd, len;
	char buf[MAX_BUF];

	len = qsnprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", _pinNumber);

	fd = open(buf, O_WRONLY);
	if( fd < 0 ){
		logError("setEdge", fd);
		return fd;
	}

	write(fd, edge, strlen(edge) + 1);

	close(fd);
#endif

	return 0;
}

int GpIoPin::setValue(uint value){
#ifdef __ARMEL__
	if( _isOut && _currentValue != value ){
		if( 0 > _ioFd ) openFd();
		if( 0 > _ioFd ) return _ioFd;

		if( value ){
			write(_ioFd, "1", 2);
		}else{
			write(_ioFd, "0", 2);
		}

		_currentValue = value;
	}
#endif

	return 0;
}
