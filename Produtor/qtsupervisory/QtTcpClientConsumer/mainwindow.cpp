#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QDateTime>
#include <QPainter>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , aguardando(Aguardando::Nada)
{
    socket = new QTcpSocket(this);
    timerGet = new QTimer(this);

    timerSilencio = new QTimer(this);
    timerSilencio->setSingleShot(true);
    timerSilencio->setInterval(150); // ms sem novos dados = resposta completa

    setupUi();

    connect(socket, &QTcpSocket::connected, this, &MainWindow::onConectado);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::onDesconectado);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::onDadosRecebidos);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onErroSocket);
#else
#endif

    connect(timerSilencio, &QTimer::timeout, this, &MainWindow::onSilencioDetectado);
    connect(timerGet, &QTimer::timeout, this, &MainWindow::onTimerGet);

    connect(btnConectar, &QPushButton::clicked, this, &MainWindow::onConectarClicked);
    connect(btnAtualizarLista, &QPushButton::clicked, this, &MainWindow::onAtualizarListaClicked);
    connect(btnIniciarRecepcao, &QPushButton::clicked, this, &MainWindow::onIniciarRecepcaoClicked);
    connect(btnPararRecepcao, &QPushButton::clicked, this, &MainWindow::onPararRecepcaoClicked);
    connect(listaMaquinas, &QListWidget::itemClicked, this, &MainWindow::onMaquinaSelecionada);

    atualizaBotoes();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QGroupBox *grupoConexao = new QGroupBox("Conexão", central);
    QHBoxLayout *layoutConexao = new QHBoxLayout;
    editIp = new QLineEdit("127.0.0.1", grupoConexao);
    btnConectar = new QPushButton("Conectar", grupoConexao);
    layoutConexao->addWidget(new QLabel("IP do servidor:"));
    layoutConexao->addWidget(editIp);
    layoutConexao->addWidget(btnConectar);
    grupoConexao->setLayout(layoutConexao);

    QGroupBox *grupoLista = new QGroupBox("Máquinas produtoras", central);
    QVBoxLayout *layoutLista = new QVBoxLayout;
    listaMaquinas = new QListWidget(grupoLista);
    btnAtualizarLista = new QPushButton("Update", grupoLista);
    layoutLista->addWidget(listaMaquinas);
    layoutLista->addWidget(btnAtualizarLista);
    grupoLista->setLayout(layoutLista);

    QGroupBox *grupoCaptura = new QGroupBox("Captura de dados", central);
    QFormLayout *formCaptura = new QFormLayout;

    spinIntervalo = new QSpinBox(grupoCaptura);
    spinIntervalo->setRange(100, 3600000);
    spinIntervalo->setSingleStep(100);
    spinIntervalo->setValue(1000);
    spinIntervalo->setSuffix(" ms");
    formCaptura->addRow("Intervalo entre leituras:", spinIntervalo);

    spinAmostras = new QSpinBox(grupoCaptura);
    spinAmostras->setRange(1, 10000);
    spinAmostras->setValue(1);
    formCaptura->addRow("Amostras por leitura:", spinAmostras);

    checkSubintervalo = new QCheckBox("Plotar apenas as últimas N amostras", grupoCaptura);
    formCaptura->addRow(checkSubintervalo);

    spinJanela = new QSpinBox(grupoCaptura);
    spinJanela->setRange(2, 100000);
    spinJanela->setValue(30);
    formCaptura->addRow("Tamanho da janela (N):", spinJanela);

    QHBoxLayout *layoutBotoesCaptura = new QHBoxLayout;
    btnIniciarRecepcao = new QPushButton("Iniciar recepção", grupoCaptura);
    btnPararRecepcao   = new QPushButton("Parar recepção", grupoCaptura);
    layoutBotoesCaptura->addWidget(btnIniciarRecepcao);
    layoutBotoesCaptura->addWidget(btnPararRecepcao);
    formCaptura->addRow(layoutBotoesCaptura);

    grupoCaptura->setLayout(formCaptura);

    serie = new QLineSeries();
    chart = new QChart();
    chart->addSeries(serie);
    chart->legend()->hide();
    chart->setTitle("Dados recebidos");

    eixoX = new QDateTimeAxis();
    eixoX->setFormat("hh:mm:ss");
    eixoX->setTitleText("Data/Hora");
    chart->addAxis(eixoX, Qt::AlignBottom);
    serie->attachAxis(eixoX);

    eixoY = new QValueAxis();
    eixoY->setTitleText("Valor");
    chart->addAxis(eixoY, Qt::AlignLeft);
    serie->attachAxis(eixoY);

    chartView = new QChartView(chart, central);
    chartView->setRenderHint(QPainter::Antialiasing);

    labelStatus = new QLabel("Desconectado", central);

    QHBoxLayout *layoutSuperior = new QHBoxLayout;
    QVBoxLayout *layoutEsquerda = new QVBoxLayout;
    layoutEsquerda->addWidget(grupoConexao);
    layoutEsquerda->addWidget(grupoLista);
    layoutEsquerda->addWidget(grupoCaptura);
    layoutEsquerda->addWidget(labelStatus);

    QWidget *widgetEsquerda = new QWidget(central);
    widgetEsquerda->setLayout(layoutEsquerda);
    widgetEsquerda->setMaximumWidth(320);

    layoutSuperior->addWidget(widgetEsquerda);
    layoutSuperior->addWidget(chartView, 1);

    QVBoxLayout *layoutPrincipal = new QVBoxLayout(central);
    layoutPrincipal->addLayout(layoutSuperior);

    resize(900, 560);
}

void MainWindow::atualizaBotoes()
{
    bool conectado = (socket->state() == QAbstractSocket::ConnectedState);
    bool recebendo = timerGet->isActive();
    bool maquinaEscolhida = !maquinaSelecionada.isEmpty();

    btnConectar->setEnabled(!conectado);
    editIp->setEnabled(!conectado);

    btnAtualizarLista->setEnabled(conectado);
    btnIniciarRecepcao->setEnabled(conectado && maquinaEscolhida && !recebendo);
    btnPararRecepcao->setEnabled(conectado && recebendo);
}

void MainWindow::log(const QString &texto)
{
    labelStatus->setText(texto);
}

void MainWindow::enviarMensagem(const QString &msg)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    socket->write((msg + "\n").toUtf8());
    socket->flush();
}

// Conexão

void MainWindow::onConectarClicked()
{
    QString ip = editIp->text().trimmed();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, "Atenção", "Informe o IP do servidor.");
        return;
    }
    log(QString("Conectando a %1:1234 ...").arg(ip));
    socket->connectToHost(ip, 1234);
}

void MainWindow::onConectado()
{
    log(QString("Conectado a %1:1234").arg(editIp->text()));
    atualizaBotoes();
    onAtualizarListaClicked();
}

void MainWindow::onDesconectado()
{
    timerGet->stop();
    log("Desconectado");
    listaMaquinas->clear();
    atualizaBotoes();
}

void MainWindow::onErroSocket(QAbstractSocket::SocketError)
{
    log(QString("Erro: %1").arg(socket->errorString()));
    atualizaBotoes();
}

// Lista de máquinas produtoras

void MainWindow::onAtualizarListaClicked()
{
    aguardando = Aguardando::ListaDeMaquinas;
    bufferRecepcao.clear();
    enviarMensagem("list");
}

void MainWindow::onMaquinaSelecionada(QListWidgetItem *item)
{
    if (!item) return;
    maquinaSelecionada = item->text();
    amostras.clear();
    serie->clear();
    log(QString("Máquina selecionada: %1").arg(maquinaSelecionada));
    atualizaBotoes();
}

// Recepção periódica de dados (get)

void MainWindow::onIniciarRecepcaoClicked()
{
    if (maquinaSelecionada.isEmpty()) {
        QMessageBox::warning(this, "Atenção", "Selecione uma máquina produtora na lista.");
        return;
    }
    timerGet->start(spinIntervalo->value());
    onTimerGet(); // primeira leitura imediata
    atualizaBotoes();
}

void MainWindow::onPararRecepcaoClicked()
{
    timerGet->stop();
    atualizaBotoes();
}

void MainWindow::onTimerGet()
{
    if (maquinaSelecionada.isEmpty() || socket->state() != QAbstractSocket::ConnectedState)
        return;

    aguardando = Aguardando::DadosDeMaquina;
    bufferRecepcao.clear();
    QString comando = QString("get %1 %2").arg(maquinaSelecionada).arg(spinAmostras->value());
    enviarMensagem(comando);
}

// Recepção de dados do socket

void MainWindow::onDadosRecebidos()
{
    bufferRecepcao += socket->readAll();
    // Reinicia o debounce: só processamos quando o servidor parar de enviar dados
    timerSilencio->start();
}

void MainWindow::onSilencioDetectado()
{
    QString texto = QString::fromUtf8(bufferRecepcao);
    QStringList linhas = texto.split('\n', Qt::SkipEmptyParts);
    bufferRecepcao.clear();

    if (linhas.isEmpty()) {
        aguardando = Aguardando::Nada;
        return;
    }

    switch (aguardando) {
    case Aguardando::ListaDeMaquinas:
        processaRespostaLista(linhas);
        break;
    case Aguardando::DadosDeMaquina:
        processaRespostaGet(linhas);
        break;
    default:
        break;
    }

    aguardando = Aguardando::Nada;
}

void MainWindow::processaRespostaLista(const QStringList &linhas)
{
    QString atual = maquinaSelecionada;
    listaMaquinas->clear();
    for (const QString &linha : linhas) {
        QString ip = linha.trimmed();
        if (!ip.isEmpty())
            listaMaquinas->addItem(ip);
    }
    // Tenta manter a seleção anterior, se ainda existir na lista
    if (!atual.isEmpty()) {
        auto itens = listaMaquinas->findItems(atual, Qt::MatchExactly);
        if (!itens.isEmpty())
            listaMaquinas->setCurrentItem(itens.first());
    }
    log(QString("Lista de máquinas atualizada (%1 encontrada(s)).").arg(listaMaquinas->count()));
}

void MainWindow::processaRespostaGet(const QStringList &linhas)
{
    int adicionadas = 0;
    for (const QString &linha : linhas) {
        QStringList partes = linha.trimmed().split(' ', Qt::SkipEmptyParts);
        if (partes.size() < 2)
            continue;

        bool ok1 = false, ok2 = false;
        qint64 timestamp = partes.at(0).toLongLong(&ok1);
        double valor = partes.at(1).toDouble(&ok2);

        if (ok1 && ok2) {
            amostras.append(QPointF(static_cast<double>(timestamp), valor));
            adicionadas++;
        }
    }

    if (adicionadas > 0) {
        redesenhaGrafico();
        log(QString("Recebida(s) %1 amostra(s) de %2.").arg(adicionadas).arg(maquinaSelecionada));
    }
}

void MainWindow::redesenhaGrafico()
{
    // Garante ordenação por tempo (o servidor deve retornar em ordem, mas por segurança...)
    std::sort(amostras.begin(), amostras.end(), [](const QPointF &a, const QPointF &b) {
        return a.x() < b.x();
    });

    QVector<QPointF> paraExibir;
    if (checkSubintervalo->isChecked()) {
        int janela = spinJanela->value();
        int inicio = qMax(0, amostras.size() - janela);
        for (int i = inicio; i < amostras.size(); ++i)
            paraExibir.append(amostras.at(i));
    } else {
        paraExibir = amostras;
    }

    serie->clear();
    double minY = 0, maxY = 1;
    qint64 minX = 0, maxX = 1;
    bool primeiro = true;

    for (const QPointF &p : paraExibir) {
        serie->append(p.x(), p.y());
        if (primeiro) {
            minY = maxY = p.y();
            minX = maxX = static_cast<qint64>(p.x());
            primeiro = false;
        } else {
            minY = qMin(minY, p.y());
            maxY = qMax(maxY, p.y());
            minX = qMin(minX, static_cast<qint64>(p.x()));
            maxX = qMax(maxX, static_cast<qint64>(p.x()));
        }
    }

    if (!primeiro) {
        if (qFuzzyCompare(minY, maxY)) { minY -= 1; maxY += 1; }
        eixoY->setRange(minY, maxY);
        eixoX->setRange(QDateTime::fromMSecsSinceEpoch(minX),
                         QDateTime::fromMSecsSinceEpoch(maxX == minX ? maxX + 1000 : maxX));
    }
}
