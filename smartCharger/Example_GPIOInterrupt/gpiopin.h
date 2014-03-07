#ifndef GPIOPIN_H
#define GPIOPIN_H

#include <QObject>

class GpIoPin : public QObject{
	Q_OBJECT

	private:
		bool	_isOut;

		int		_ioFd;

		uint	_currentValue;
		uint	_pinNumber;

		int		exportPin();
		void	openFd();
		int		setDirection(bool out);

	public:
		explicit GpIoPin(uint pinNumber, bool isOut = true, QObject *parent = 0);
		~GpIoPin();

		int	getValue(uint* value);

		int setEdge(char *edge);
		int setValue(uint value);

	signals:

	public slots:
};

#endif // GPIOPIN_H