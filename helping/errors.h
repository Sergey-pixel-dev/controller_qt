#ifndef ERRORS_H
#define ERRORS_H

enum class ConnectResult {
    Success = 0,
    AlreadyConnected = -1,
    DataNotReceived = -2,
    ManagerStartError = -3,
    PortOpenError = -4
};

enum class MeasurementResult {
    Success = 0,
    NotConnected = -1,
    SignalNotSet = -2,
    AlreadyRunning = -3,
    StartError = -4,
    Timeout = -5

};

enum class SignalResult { Success = 0, NotConnected = -1, InvalidParameters = -2, SendError = -3 };

#endif // ERRORS_H
