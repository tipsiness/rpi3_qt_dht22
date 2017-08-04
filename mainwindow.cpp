#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCoreApplication>
#include <QString>

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXTIMINGS		82
#define DHTPIN			28
#define POWER			27

int dht22_data[] = {0, 0, 0, 0, 0};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	wiringPiSetup();

	pinMode(POWER, OUTPUT);

	m_timer = new QTimer(this);
	connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
	m_timer->start(2000);

	m_label = new QLabel(QString("Temperature & Humidity"), this);
	m_label->setGeometry(QRect(QPoint(100, 50), QSize(250, 100)));

	QFont font = m_label->font();
	font.setPointSize(20);
	font.setBold(true);
	m_label->setFont(font);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onTimer() {
	// Read & Display CDS Cell Value
	digitalWrite(POWER, HIGH);
	int ret = readDHT22Data();
	QString qStr = QString("Temp: ") + QString::number(this->temp) + QString("(C)\nHumi: ") + QString::number(this->humi) + QString("%");
	m_label->setText(qStr);
}

unsigned int MainWindow::readDHT22Data() {
	uint8_t laststate = HIGH;
	uint8_t counter = 0;
	uint8_t i, j = 0;

	dht22_data[0] = dht22_data[1] = dht22_data[2] = dht22_data[3] = dht22_data[4] = 0;

	/* Single-Wire Two-Way Communication */
	pinMode(DHTPIN, OUTPUT);

	/* MCU->DHT, start signal */
	digitalWrite(DHTPIN, HIGH);
	delay(1);

	/* MCU pull down, DHT detect a start signal from MCU */
	digitalWrite(DHTPIN, LOW);
	delay(2);

	/* MCU pull up to receive response signal from DHT */
	digitalWrite(DHTPIN, HIGH);
	delayMicroseconds(40);

	/* MCU read response data consist of temperature and humidity */
	pinMode(DHTPIN, INPUT);
	delayMicroseconds(1);

	for (i = 1; i <= MAXTIMINGS; i++) {
		counter = 0;
		while (digitalRead(DHTPIN) == laststate) {
			counter++;
			delayMicroseconds(1);

			if (counter == 75)
				break;
		}
		laststate = digitalRead(DHTPIN);

		if ((i > 3) && (i % 2 == 0)) {
			dht22_data[j/8] <<= 1;		// read bit '0'
			if (counter > 10)
				dht22_data[j/8] |= 1;	// read bit '1'
			j++;
		}
	}

	if ((j >= 40) && (dht22_data[4] == ((dht22_data[0] + dht22_data[1] + dht22_data[2] + dht22_data[3]) & 0xFF))) {
		float t, h;
		h = ((dht22_data[0] << 8) | dht22_data[1]) / 10.0;
		t = ((dht22_data[2] << 8) | dht22_data[3]) / 10.0;
		//if ((dht22_data[2] & 0x80) != 0) t *= -1;

		printf("Humidity = %.1f %% Temperature = %.1f *C\n", h, t);

		this->temp = t;
		this->humi = h;

		digitalWrite(POWER, LOW);
		return 1;
	} else {
		printf("Data not good, skip (read bytes: %d).\n", j);
		digitalWrite(POWER, LOW);
		return 0;
	}
}





