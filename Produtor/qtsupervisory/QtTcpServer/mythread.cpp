#include "mythread.h"
#include <vector>

using namespace std;

MyThread::MyThread(int ID, QObject *parent, DataStorage* storage) :
  QThread (parent)
{
  this->socketDescriptor = ID;
  this->storage = storage;
}

void MyThread::run(){
  str = QString("<i>") +
      QString().setNum(socketDescriptor) +
      " <font color=\"red\">Connected! starting thread</blue></i>";
  emit message(str);
  socket = new QTcpSocket();
  if(!socket->setSocketDescriptor(this->socketDescriptor)){
    emit error(socket->error());
    return;
  }
  // direct connection slot invocado diretamente
  connect(socket,SIGNAL(readyRead()),this,SLOT(readyRead()),Qt::DirectConnection);
  connect(socket,SIGNAL(disconnected()),this,SLOT(disconnected()),Qt::DirectConnection);

  str = QString().setNum(socketDescriptor) + " client connected";
  emit message(str);
  peerAddress = socket->peerAddress();
  exec();
}

void MyThread::readyRead(){
  vector<Entry> entry;
  vector<QHostAddress> hostList;
  Entry value;
  qint64 msecdate;
  unsigned int nsamples;
  QString cmd;

  QString str = socket->readLine().replace("\n","").replace("\r","");
  emit message(str);
  QStringList list;

  list = str.split(" ");

  cmd = list.at(0);

  // lista os hosts produtores de dados
  if(cmd == "list"){
    hostList = storage->getHostList();
    for(int i=0; i<hostList.size(); i++){
      socket->write(hostList[i].toString().toStdString().c_str());
      socket->write("\r\n");
    }
  }

  // lista as entradas de um dado host
  else if(cmd == "get"){
    if(list.size() == 3){
      cmd = list.at(1);
      str = list.at(2);
      nsamples = str.toUInt();
      QHostAddress address(cmd);
      if(!address.isNull()){
        // recupera entradas para um dado endereco
        entry = storage->getData(address, nsamples);
        // mostra as entradas para o cliente
        for(int i=0; i<entry.size(); i++){
          value = entry[i];
          socket->write(QString().number(value.theTime).toStdString().c_str());
          socket->write(" ");
          socket->write(QString().number(value.measurement).toStdString().c_str());
          socket->write("\n");
        }
      }
    }
  }

  // insere dados de telemetria
  else if(cmd == "set"){
    if(list.size() == 3){
      bool ok;
      cmd = list.at(1);
      msecdate = cmd.toLongLong(&ok);
      if(ok){
        cmd = list.at(2);
        value.measurement = cmd.toFloat(&ok);
        if(ok){
          storage->addData(socket->peerAddress(),msecdate,
                           value.measurement);
        }
      }
    }
  }

}

void MyThread::disconnected(){
  str = QString("<i>") + QString().setNum(socketDescriptor)+
      " <font color=\"red\">Disconnected</font></i>";
  emit message(str);
  socket->deleteLater();
  // diz a thread para parar
  exit(0);
}
