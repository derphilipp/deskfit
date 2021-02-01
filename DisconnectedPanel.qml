import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.2

Item {
  signal startDiscovery

  id: root

  Timer {
    id: timer
    running: true
    repeat: true
    interval: 100
    onTriggered: {
        root.startDiscovery()
    }
  }


  Button {
    anchors.centerIn: parent
    text: qsTr("Start Discovery")
    onClicked: root.startDiscovery()
  }
}
