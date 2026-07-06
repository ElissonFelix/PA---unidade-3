#include "mainwindow.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    socket = new QTcpSocket(this);
    timerEnvio = new QTimer(this);

    setupUi();

    connect(socket, &QTcpSocket::connected, this, &MainWindow::onConectado);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::onDesconectado);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onErroSocket);
#else

#endif

    connect(timerEnvio, &QTimer::timeout, this, &MainWindow::onEnviarDado);

    connect(btnConectar, &QPushButton::clicked, this, &MainWindow::onConectarClicked);
    connect(btnIniciar, &QPushButton::clicked, this, &MainWindow::onIniciarClicked);
    connect(btnFinalizar, &QPushButton::clicked, this, &MainWindow::onFinalizarClicked);

    atualizaBotoes();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QGroupBox *grupoConfig = new QGroupBox("Configuração", central);
    QFormLayout *formLayout = new QFormLayout;

    editIp = new QLineEdit("127.0.0.1", grupoConfig);
    formLayout->addRow("IP do servidor:", editIp);

    spinMin = new QDoubleSpinBox(grupoConfig);
    spinMin->setRange(-1000000, 1000000);
    spinMin->setValue(0);
    formLayout->addRow("Valor mínimo:", spinMin);

    spinMax = new QDoubleSpinBox(grupoConfig);
    spinMax->setRange(-1000000, 1000000);
    spinMax->setValue(100);
    formLayout->addRow("Valor máximo:", spinMax);

    spinIntervalo = new QSpinBox(grupoConfig);
    spinIntervalo->setRange(100, 3600000);
    spinIntervalo->setSingleStep(100);
    spinIntervalo->setValue(1000);
    spinIntervalo->setSuffix(" ms");
    formLayout->addRow("Intervalo de envio:", spinIntervalo);

    grupoConfig->setLayout(formLayout);

    QHBoxLayout *layoutBotoes = new QHBoxLayout;
    btnConectar  = new QPushButton("Conectar", central);
    btnIniciar   = new QPushButton("Iniciar transmissão", central);
    btnFinalizar = new QPushButton("Finalizar transmissão", central);
    layoutBotoes->addWidget(btnConectar);
    layoutBotoes->addWidget(btnIniciar);
    layoutBotoes->addWidget(btnFinalizar);

    labelStatus = new QLabel("Desconectado", central);
    textLog = new QTextEdit(central);
    textLog->setReadOnly(true);

    QVBoxLayout *layoutPrincipal = new QVBoxLayout(central);
    layoutPrincipal->addWidget(grupoConfig);
    layoutPrincipal->addLayout(layoutBotoes);
    layoutPrincipal->addWidget(labelStatus);
    layoutPrincipal->addWidget(new QLabel("Dados enviados:", central));
    layoutPrincipal->addWidget(textLog);

    resize(480, 420);
}

void MainWindow::atualizaBotoes()
{
    bool conectado = (socket->state() == QAbstractSocket::ConnectedState);
    bool transmitindo = timerEnvio->isActive();

    btnConectar->setEnabled(!conectado);
    editIp->setEnabled(!conectado);

    btnIniciar->setEnabled(conectado && !transmitindo);
    btnFinalizar->setEnabled(conectado && transmitindo);
}

void MainWindow::log(const QString &texto)
{
    textLog->append(texto);
}

void MainWindow::enviarMensagem(const QString &msg)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        log("[erro] não conectado ao servidor.");
        return;
    }
    QByteArray dados = (msg + "\n").toUtf8();
    socket->write(dados);
    socket->flush();
}

void MainWindow::onConectarClicked()
{
    QString ip = editIp->text().trimmed();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, "Atenção", "Informe o IP do servidor.");
        return;
    }
    labelStatus->setText(QString("Conectando a %1:1234 ...").arg(ip));
    socket->connectToHost(ip, 1234);
}

void MainWindow::onConectado()
{
    labelStatus->setText(QString("Conectado a %1:1234").arg(editIp->text()));
    log("[info] conexão estabelecida.");
    atualizaBotoes();
}

void MainWindow::onDesconectado()
{
    timerEnvio->stop();
    labelStatus->setText("Desconectado");
    log("[info] conexão encerrada.");
    atualizaBotoes();
}

void MainWindow::onErroSocket(QAbstractSocket::SocketError)
{
    log(QString("[erro] %1").arg(socket->errorString()));
    labelStatus->setText("Erro de conexão");
    atualizaBotoes();
}

void MainWindow::onIniciarClicked()
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "Atenção", "Conecte-se ao servidor antes de iniciar a transmissão.");
        return;
    }
    if (spinMin->value() > spinMax->value()) {
        QMessageBox::warning(this, "Atenção", "O valor mínimo não pode ser maior que o máximo.");
        return;
    }
    timerEnvio->start(spinIntervalo->value());
    onEnviarDado(); // envia imediatamente a primeira amostra
    atualizaBotoes();
}

void MainWindow::onFinalizarClicked()
{
    timerEnvio->stop();
    atualizaBotoes();
    log("[info] transmissão finalizada pelo usuário.");
}

void MainWindow::onEnviarDado()
{
    qint64 agoraMs = QDateTime::currentMSecsSinceEpoch();

    double minimo = spinMin->value();
    double maximo = spinMax->value();
    double valor = minimo + QRandomGenerator::global()->generateDouble() * (maximo - minimo);

    QString comando = QString("set %1 %2").arg(agoraMs).arg(valor, 0, 'f', 2);
    enviarMensagem(comando);
    log(QString("[enviado] %1").arg(comando));
}
