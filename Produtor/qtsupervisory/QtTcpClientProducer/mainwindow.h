#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>

class QLineEdit;
class QPushButton;
class QTextEdit;
class QLabel;
class QSpinBox;
class QDoubleSpinBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConectarClicked();
    void onIniciarClicked();
    void onFinalizarClicked();
    void onEnviarDado();

    void onConectado();
    void onDesconectado();
    void onErroSocket(QAbstractSocket::SocketError socketError);

private:
    void setupUi();
    void enviarMensagem(const QString &msg);
    void log(const QString &texto);
    void atualizaBotoes();

    QTcpSocket *socket;
    QTimer *timerEnvio;

    // Widgets de configuração
    QLineEdit      *editIp;
    QDoubleSpinBox *spinMin;
    QDoubleSpinBox *spinMax;
    QSpinBox       *spinIntervalo; // em milissegundos

    // Botões de ação
    QPushButton *btnConectar;
    QPushButton *btnIniciar;
    QPushButton *btnFinalizar;

    // Saída
    QTextEdit *textLog;
    QLabel    *labelStatus;
};

#endif // MAINWINDOW_H
