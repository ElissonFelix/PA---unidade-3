#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include <QVector>
#include <QPointF>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_USE_NAMESPACE
#endif

class QLineEdit;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QSpinBox;
class QCheckBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConectarClicked();
    void onAtualizarListaClicked();
    void onIniciarRecepcaoClicked();
    void onPararRecepcaoClicked();
    void onMaquinaSelecionada(QListWidgetItem *item);
    void onTimerGet();

    void onConectado();
    void onDesconectado();
    void onErroSocket(QAbstractSocket::SocketError socketError);
    void onDadosRecebidos();
    void onSilencioDetectado();

private:
    // Estados do que estamos esperando como resposta do servidor
    enum class Aguardando { Nada, ListaDeMaquinas, DadosDeMaquina };

    void setupUi();
    void atualizaBotoes();
    void enviarMensagem(const QString &msg);
    void log(const QString &texto);

    void processaRespostaLista(const QStringList &linhas);
    void processaRespostaGet(const QStringList &linhas);
    void redesenhaGrafico();

    QTcpSocket *socket;
    QTimer *timerGet;
    QTimer *timerSilencio; // debounce: considera a resposta completa após um curto período sem novos dados

    Aguardando aguardando;
    QByteArray bufferRecepcao;

    QString maquinaSelecionada;
    QVector<QPointF> amostras; // x = timestamp (ms desde epoch), y = valor

    // --- Widgets ---
    QLineEdit   *editIp;
    QPushButton *btnConectar;
    QListWidget *listaMaquinas;
    QPushButton *btnAtualizarLista;
    QPushButton *btnIniciarRecepcao;
    QPushButton *btnPararRecepcao;
    QSpinBox    *spinIntervalo;   // intervalo entre comandos get
    QSpinBox    *spinAmostras;    // quantidade de amostras solicitadas por get
    QCheckBox   *checkSubintervalo; // se marcado, plota apenas as últimas N amostras
    QSpinBox    *spinJanela;      // tamanho da janela (últimas N amostras) quando checkSubintervalo ativo
    QLabel      *labelStatus;

    QChartView  *chartView;
    QChart      *chart;
    QLineSeries *serie;
    QDateTimeAxis *eixoX;
    QValueAxis    *eixoY;
};

#endif // MAINWINDOW_H
