#include "deskfit.h"

#include <QMetaEnum>
#include <cmath>
#include <iostream>

#include <fstream>
#include <string>
#include <ctime>
#include <filesystem>

const std::array<double, 9> SPEED_CONVERSION{ 0.0, 0.7, 1.5, 2.4, 3.3,
    4.3, 5.2, 6.1, 7.1 };

DeskFit::DeskFit(QObject* parent)
    : QObject(parent)
    , m_controller(nullptr)
    , m_service(nullptr)
    , m_connectionStatus(ConnectionStatus::DisconnectedStatus)
    , m_deviceStatus(DeviceStatus::StoppedStatus)
    , m_distance(0.0)
    , m_countdown(0)
    , m_calories(0)
    , m_steps(0)
    , m_speed(0.0)
    , m_time(0)
    , m_realSpeed(0.0)
    , m_total_distance(0.0)
    , m_total_calories(0)
    , m_total_steps(0)
    , m_total_time(0.0)
{
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(5000);
    connect(m_discoveryAgent,
        &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
        this,
        &DeskFit::deviceDiscovered);
    connect(m_discoveryAgent,
        QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(
            &QBluetoothDeviceDiscoveryAgent::error),
        this,
        &DeskFit::deviceScanError);
    connect(m_discoveryAgent,
        &QBluetoothDeviceDiscoveryAgent::finished,
        this,
        &DeskFit::deviceScanFinished);

    m_fetchStatusTimer.setInterval(500);
    connect(&m_fetchStatusTimer, &QTimer::timeout, this, &DeskFit::fetchStatus);
    connect(this, &DeskFit::connectionStatusChanged, [this]() {
        if (this->m_connectionStatus == ConnectionStatus::ConnectedStatus) {
            this->m_fetchStatusTimer.start();
        } else {
            this->m_fetchStatusTimer.stop();
        }
    });
}

DeskFit::ConnectionStatus
DeskFit::connectionStatus() const
{
    return m_connectionStatus;
}

DeskFit::DeviceStatus
DeskFit::deviceStatus() const
{
    return m_deviceStatus;
}

void DeskFit::deviceDiscovered(const QBluetoothDeviceInfo& info)
{
    if (info.address() == QBluetoothAddress(m_peripheralUuid)) {
        m_deviceInfo = info;
        m_connectionStatus = ConnectionStatus::DiscoveredStatus;
        emit connectionStatusChanged();
        connectPeripheral();
    }else {
	    std::cout<<info.address().toString().toStdString()<< std::endl;
    }

}

void DeskFit::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError)
        setUpdate("The Bluetooth adaptor is powered off, power it on before doing "
                  "discovery.");
    else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError)
        setUpdate("Writing or reading from the device resulted in an error.");
    else {
        static QMetaEnum qme = m_discoveryAgent->metaObject()->enumerator(
            m_discoveryAgent->metaObject()->indexOfEnumerator("Error"));
        setUpdate("Error: " + QLatin1String(qme.valueToKey(error)));
    }

    m_connectionStatus = ConnectionStatus::DisconnectedStatus;
    emit connectionStatusChanged();
}

void DeskFit::deviceScanFinished()
{
    if (m_connectionStatus == ConnectionStatus::DiscoveryStatus) {
        m_connectionStatus = ConnectionStatus::DisconnectedStatus;
        emit connectionStatusChanged();
    }
}

void DeskFit::connectPeripheral()
{
    m_controller = QLowEnergyController::createCentral(m_deviceInfo);
    connect(m_controller,
        &QLowEnergyController::connected,
        this,
        &DeskFit::deviceConnected);
    connect(
        m_controller,
        QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
        this,
        &DeskFit::errorReceived);
    connect(m_controller,
        &QLowEnergyController::disconnected,
        this,
        &DeskFit::deviceDisconnected);
    connect(m_controller,
        &QLowEnergyController::discoveryFinished,
        this,
        &DeskFit::serviceScanDone);

    m_controller->setRemoteAddressType(QLowEnergyController::RandomAddress);
    m_controller->connectToDevice();

    m_connectionStatus = ConnectionStatus::ConnectingStatus;
    emit connectionStatusChanged();
}

void DeskFit::deviceConnected()
{
    m_controller->discoverServices();

    qDebug() << "device connected";
}

void DeskFit::errorReceived(QLowEnergyController::Error /*error*/)
{
    qWarning() << "Error: " << m_controller->errorString();
    setUpdate(QString("Back\n(%1)").arg(m_controller->errorString()));
}

void DeskFit::serviceScanDone()
{
    m_service = m_controller->createServiceObject(
        QBluetoothUuid(QStringLiteral("{0000fff0-0000-1000-8000-00805f9b34fb}")));
    connect(m_service,
        &QLowEnergyService::stateChanged,
        this,
        &DeskFit::serviceDetailsDiscovered);
    connect(m_service,
        &QLowEnergyService::characteristicChanged,
        this,
        &DeskFit::characteristicChanged);
    m_service->discoverDetails();
}

void DeskFit::deviceDisconnected()
{
    qWarning() << "Disconnected from device";
    m_connectionStatus = ConnectionStatus::DisconnectedStatus;
    emit connectionStatusChanged();
}

void DeskFit::serviceDetailsDiscovered(QLowEnergyService::ServiceState newState)
{
    if (newState != QLowEnergyService::ServiceDiscovered) {
        // do not hang in "Scanning for characteristics" mode forever
        // in case the service discovery failed
        // We have to queue the signal up to give UI time to even enter
        // the above mode
        //        if (newState != QLowEnergyService::DiscoveringServices) {
        //            QMetaObject::invokeMethod(this, "characteristicsUpdated",
        //                                      Qt::QueuedConnection);
        //        }
        return;
    }

    m_command = m_service->characteristic(
        QBluetoothUuid(QStringLiteral("{0000fff2-0000-1000-8000-00805f9b34fb}")));

    m_status = m_service->characteristic(
        QBluetoothUuid(QStringLiteral("{0000fff1-0000-1000-8000-00805f9b34fb}")));
    auto notification = m_status.descriptor(
        QBluetoothUuid(QStringLiteral("{00002902-0000-1000-8000-00805f9b34fb}")));
    m_service->writeDescriptor(notification, QByteArray::fromHex("0100"));

    m_connectionStatus = ConnectionStatus::ConnectedStatus;
    emit connectionStatusChanged();

    qDebug() << "fully connected";
}

void DeskFit::characteristicChanged(const QLowEnergyCharacteristic& characteristic,
    const QByteArray& newValue)
{
    if (characteristic != m_status) {
        return;
    }

#if 0
    QStringList printStringList;
    for (const auto &byte: newValue) {
        printStringList.append(QString::number( static_cast<quint8>(byte), 16 ));
    }
    qDebug() << printStringList.join(":");
#endif
    updateDeviceStatus(newValue);
}

QString
DeskFit::peripheralUuid() const
{
    return m_peripheralUuid;
}

QString
DeskFit::update() const
{
    return m_update;
}

double DeskFit::distance() const
{
    return m_distance;
}

double DeskFit::total_distance() const
{
    return m_total_distance;
}


int DeskFit::countdown() const
{
    return m_countdown;
}

int DeskFit::calories() const
{
    return m_calories;
}

int DeskFit::total_calories() const
{
    return m_total_calories;
}

int DeskFit::steps() const
{
    return m_steps;
}

int DeskFit::total_steps() const
{
    return m_total_steps;
}

double
DeskFit::speed() const
{
    return m_speed;
}

int DeskFit::time() const
{
    return m_time;
}

int DeskFit::total_time() const
{
    return m_total_time;
}

double DeskFit::realSpeed() const
{
    return m_realSpeed;
}

void DeskFit::startDeviceDiscovery()
{
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);

    if (m_discoveryAgent->isActive()) {
        m_connectionStatus = ConnectionStatus::DiscoveryStatus;
        emit connectionStatusChanged();
    }
}

void DeskFit::disconnectDevice()
{
    if (!m_controller) {
        return;
    }

    m_controller->deleteLater();
    m_controller = nullptr;

    if (m_service) {
        m_service->deleteLater();
        m_service = nullptr;
    }

    m_connectionStatus = ConnectionStatus::DisconnectedStatus;
    emit connectionStatusChanged();
}

void DeskFit::setPeripheralUuid(const QString& peripheralUuid)
{
    if (m_peripheralUuid == peripheralUuid)
        return;

    m_peripheralUuid = peripheralUuid;
    emit peripheralUuidChanged(m_peripheralUuid);
}

void DeskFit::start()
{
    if (m_connectionStatus != ConnectionStatus::ConnectedStatus) {
        qWarning() << "device not connected";
        return;
    }
    m_service->writeCharacteristic(m_command, createCommand(Command::Start));
}


void DeskFit::writeLog(){
    // Start write progress CSV hack
    if (!std::filesystem::exists("log.csv")) {
        std::ofstream outfile;
        outfile.open("log.csv");
        outfile << "DateTime;Steps;Seconds;Distance;Calories;" << std::endl;
        outfile.close();
    }
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    std::time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,sizeof(buffer),"%d-%m-%Y %H:%M:%S",timeinfo);
    std::string str(buffer);

    std::ofstream outfile;
    outfile.open("log.csv", std::ios_base::app);
//    outfile << "DateTime;Time;Steps;Distance;Calories" << std::endl;
    outfile << str << ";" << m_steps << ";" << m_time/1000 << ";" << m_distance << ";" << m_calories << ";" << std::endl;
    outfile.close();
}


void DeskFit::stop()
{
    if (m_connectionStatus != ConnectionStatus::ConnectedStatus) {
        qWarning() << "device not connected";
        return;
    }
    // Start write progress CSV hack
    if (!std::filesystem::exists("walking.csv")) {
        std::ofstream outfile;
        outfile.open("walking.csv");
        outfile << "DateTime;Steps;Seconds;Distance;Calories;" << std::endl;
        outfile.close();
    }


    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    std::time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,sizeof(buffer),"%d-%m-%Y %H:%M:%S",timeinfo);
    std::string str(buffer);

    std::ofstream outfile;
    outfile.open("walking.csv", std::ios_base::app);
//    outfile << "DateTime;Time;Steps;Distance;Calories" << std::endl;
    outfile << str << ";" << m_steps << ";" << m_time/1000 << ";" << m_distance << ";" << m_calories << ";" << std::endl;
    outfile.close();

    // End write progress CSV hack

    m_service->writeCharacteristic(m_command, createCommand(Command::Stop));
}

void DeskFit::clear()
{
    m_total_time=m_time;
    m_total_steps=m_steps;
    m_total_calories=m_calories;
    m_total_distance=m_distance;
}

void DeskFit::pause()
{
    if (m_connectionStatus != ConnectionStatus::ConnectedStatus) {
        qWarning() << "device not connected";
        return;
    }
    m_service->writeCharacteristic(m_command, createCommand(Command::Pause));
}

void DeskFit::setSpeed(int level)
{
    if (m_connectionStatus != ConnectionStatus::ConnectedStatus) {
        qWarning() << "device not connected";
        return;
    }
    m_service->writeCharacteristic(
        m_command,
        createCommand(Command::SetSpeed,
            static_cast<quint8>(qMin(qMax(level, 0), 80))));
}

void DeskFit::fetchStatus()
{
    if (m_connectionStatus != ConnectionStatus::ConnectedStatus) {
        qWarning() << "device not connected";
        return;
    }
    m_service->writeCharacteristic(m_command,
        createCommand(Command::FetchStatus));
}

QByteArray
DeskFit::createCommand(const DeskFit::Command command, quint8 value)
{
    QByteArray data;

    data.append("\xf0\xc3\x03");
    data.append(static_cast<char>(command));
    data.append(static_cast<char>(value));
    data.append(static_cast<char>(0x00));

    // calculate checksum
    quint32 total = 0u;
    for (const char c : data) {
        total += static_cast<quint32>(c);
    }
    data.append(static_cast<char>(total & 0xFFu));

    return data;
}

void DeskFit::updateDeviceStatus(const QByteArray& data)
{
    char statusByte = data[14];
    DeviceStatus status;

    switch (statusByte) {
    case 0x1:
        status = DeviceStatus::StoppedStatus;
        break;
    case 0x2:
        status = DeviceStatus::StartingStatus;
        break;
    case 0x3:
        status = DeviceStatus::RunningStatus;
        break;
    case 0x4:
        status = DeviceStatus::PausedStatus;
        break;
    default:
        status = DeviceStatus::UnknownStatus;
        break;
    }
    if (status != m_deviceStatus) {
        m_deviceStatus = status;
        emit deviceStatusChanged();
    }

    int countdown = 0;
    if (status == DeviceStatus::UnknownStatus) {
        return;
    } else if (status == DeviceStatus::StartingStatus) {
        countdown = static_cast<int>(data[16]);
    }

    int msecs = static_cast<int>(data[3]) * 1000 + static_cast<int>(data[4]) * 60000;
    double km = static_cast<double>(data[5]) + static_cast<double>(data[6]) * 0.01;
    int calories = static_cast<int>(data[7]) * 100 + static_cast<int>(data[8]);
    double speed = static_cast<int>(data[10]) * 0.1;
    int steps = static_cast<int>(data[12]) * 100 + static_cast<int>(data[13]);

    // convert speed steps into real speed in km/h
    int lower = std::max(0, static_cast<int>(std::floor(speed)));
    int upper = std::min(8, static_cast<int>(std::ceil(speed)));
    double minSpeed = SPEED_CONVERSION[static_cast<std::size_t>(lower)];
    double maxSpeed = SPEED_CONVERSION[static_cast<std::size_t>(upper)];
    double realSpeed = (maxSpeed - minSpeed) * (speed - static_cast<double>(lower)) + minSpeed;

    if (msecs != m_time) {
        if (msecs > m_time){
            // If we don't have a new start, just add the time difference
            // Time passed -> Add the difference
            // If this is not the case we have a new start and/or a rollaround. Then do nothing for a Moment
            m_total_time+=(msecs - m_time);
            emit totaltimeChanged(m_total_time);
        }
        m_time = msecs;
        emit timeChanged(msecs);
    }
    if (!qFuzzyCompare(km, m_distance)) {
        if (km > m_distance){
            m_total_distance+=(km-m_distance);
            emit totaldistanceChanged(m_total_distance);
        }
        m_distance = km;
        emit distanceChanged(km);
    }
    if (calories != m_calories) {
        if (calories > m_calories) {
            m_total_calories+=(calories-m_calories);
            emit totalcaloriesChanged(m_total_calories);
        }
        m_calories = calories;
        emit caloriesChanged(calories);
    }
    if (!qFuzzyCompare(speed, m_speed)) {
        m_speed = speed;
        emit speedChanged(speed);
    }
    if (countdown != m_countdown) {
        m_countdown = countdown;
        emit countdownChanged(countdown);
    }
    if (steps != m_steps) {
        if (steps > m_steps){
            m_total_steps+=(steps-m_steps);
            emit totalstepsChanged(m_total_steps);
        }
        m_steps = steps;
        emit stepsChanged(steps);
    }
    if (!qFuzzyCompare(realSpeed, m_realSpeed)) {
        m_realSpeed = realSpeed;
        emit realSpeedChanged(realSpeed);
    }
}

void DeskFit::setUpdate(const QString& update)
{
    if (m_update == update)
        return;

    m_update = update;
    emit updateChanged(m_update);
}
