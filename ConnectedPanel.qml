import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import deskfit 1.0

Item {
  id: root
  property DeskFit deskfit
  width: 400
  height: 600

  QtObject {
    id: d

    function msToTime(s) {
      var ms = s % 1000
      s = (s - ms) / 1000
      var secs = s % 60
      s = (s - secs) / 60
      var mins = s % 60
      var hrs = (s - mins) / 60

      return hrs + ':' + ("0" + mins).slice(-2) + ':' + ("0" + secs).slice(-2)
    }
}

  ColumnLayout {
    id: columnLayout
    anchors.fill: parent
    anchors.margins: 5

  GridLayout {
    id: grid
    columns: 4
    Layout.fillWidth: true
    anchors.fill: parent
    anchors.margins: 25
    Layout.margins: 15
    Layout.alignment: Qt.AlignHCenter
    Layout.preferredWidth: 800

    Label {
      Layout.alignment: Qt.AlignLeft
      text: qsTr("Time")
      font.pixelSize: 30
  }

    Label {
      Layout.alignment: Qt.AlignRight
      text: qsTr("%1").arg(d.msToTime(deskfit.time))
      font.pixelSize: 30
  }
    Label {
      Layout.alignment: Qt.AlignLeft
      text: qsTr("Total")
      font.pixelSize: 30
  }
    Label {
      Layout.alignment: Qt.AlignRight
      text: qsTr("%1").arg(d.msToTime(deskfit.total_time))
      font.pixelSize: 30
  }


    Label {
      Layout.alignment: Qt.AlignLeft
      text: qsTr("Distance")
      font.pixelSize: 30
    }
    Label {
      Layout.alignment: Qt.AlignRight
      text: qsTr("%1 km").arg(deskfit.distance.toFixed(2))
      font.pixelSize: 30
    }

    Label {
      Layout.alignment: Qt.AlignLeft
      text: qsTr("Total")
      font.pixelSize: 30
  }

    Label {
      Layout.alignment: Qt.AlignRight
      text: qsTr("%1 km").arg(deskfit.total_distance.toFixed(2))
      font.pixelSize: 30
    }


    Label {
      Layout.alignment: Qt.AlignLeft
      text: qsTr("Calories")
      font.pixelSize: 30
    }
    Label {
      Layout.alignment: Qt.AlignRight
      text: qsTr("%1").arg(deskfit.calories)
      font.pixelSize: 30
    }

    Label {
      Layout.alignment: Qt.AlignLeft
      text: qsTr("Total")
      font.pixelSize: 30
    }
    Label {
      Layout.alignment: Qt.AlignRight
      text: qsTr("%1").arg(deskfit.total_calories)
      font.pixelSize: 30
    }

    Label {
      Layout.alignment: Qt.AlignLeft
      text: qsTr("Steps")
      font.pixelSize: 30
    }
    Label {
      Layout.alignment: Qt.AlignRight
      text: qsTr("%1").arg(deskfit.steps)
      font.pixelSize: 30
  }

    Label {
      Layout.alignment: Qt.AlignLeft
      text: qsTr("Total")
      font.pixelSize: 30
  }

    Label {
      Layout.alignment: Qt.AlignRight
      text: qsTr("%1").arg(deskfit.total_steps)
      font.pixelSize: 30
    }


    Label {
      Layout.alignment: Qt.AlignHCenter
      Layout.columnSpan:4
      text: qsTr("Speed: %1 km/h").arg(deskfit.realSpeed)
      font.pixelSize: 30
    }
}


    Item {
      width: 1
      height: 1
      Layout.fillHeight: true
    }

    RowLayout {
      Layout.fillWidth: true
      Layout.fillHeight: false

      Button {
        id: decreaseSpeedButton
        Layout.preferredWidth: 40
        Layout.fillHeight: true
        text: "-"
        onClicked: speedSlider.value = (Math.floor(
                                          speedSlider.value / 10) - 1) * 10
      }

      Slider {
        id: speedSlider
        Layout.fillWidth: true
        from: 10
        to: 80
        stepSize: 1
        live: false
        value: deskfit.speed * 10

        //                background: Item {
        //                    width: speedSlider.availableWidth
        //                    height: implicitHeight
        //                    implicitHeight: 100

        //                    Image {
        //                        anchors.fill: parent
        //                        source: "./icons/speed_1.png"
        //                        mipmap: true
        //                        fillMode: Image.PreserveAspectFit
        //                        visible: true
        //                    }
        //                }
        onValueChanged: deskfit.setSpeed(value)
      }

      Button {
        id: increaseSpeedButton
        Layout.preferredWidth: 40
        Layout.fillHeight: true
        text: "+"
        onClicked: speedSlider.value = (Math.floor(
                                          speedSlider.value / 10) + 1) * 10
      }
    }

    RowLayout {
      enabled: deskfit.connectionStatus == DeskFit.ConnectedStatus

      Item {
        Layout.preferredWidth: 50
        width: 1
        height: 1
      }

      Repeater {
        model: 8
        Button {
          id: speedButton
          property int speed: index + 1
          Layout.fillWidth: true
          text: (index + 1)
          checked: (speed <= deskfit.speed) && (deskfit.speed < (speed + 1))

          onClicked: deskfit.setSpeed(speed * 10)
          onCheckedChanged: {
            if (!checked) {
              checkable = false
            }
          }
        }
      }

      Item {
        Layout.preferredWidth: 50
        width: 1
        height: 1
      }
    }

    Item {
      width: 1
      height: 1
      Layout.fillHeight: true
    }

    Button {
      Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
      Layout.preferredHeight: 100
      visible: deskfit.deviceStatus == DeskFit.StoppedStatus
               || deskfit.deviceStatus == DeskFit.PausedStatus

      text: deskfit.deviceStatus == DeskFit.PausedStatus ? qsTr(
                                                             "Continue") : qsTr(
                                                             "Start")
      onClicked: deskfit.start()
    }

    RowLayout {
      id: rowLayout
      width: 100
      height: 100
      Layout.fillWidth: true
      visible: deskfit.deviceStatus === DeskFit.RunningStatus

      Item {
        width: 1
        height: 1
        Layout.fillWidth: true
      }

      Button {
        text: qsTr("Stop")
        Layout.preferredHeight: 100
        onClicked: deskfit.stop()
      }

      Button {
        text: qsTr("Pause")
        Layout.preferredHeight: 100
        onClicked: deskfit.pause()
      }

      Button {
        text: qsTr("Clear")
        Layout.preferredHeight: 100
        onClicked: deskfit.clear()
      }

      Item {
        id: element1
        width: 1
        height: 1
        Layout.fillWidth: true
      }
    }

    Label {
      Layout.alignment: Qt.AlignHCenter
      visible: deskfit.deviceStatus == DeskFit.StartingStatus
      text: deskfit.countdown
      font.pixelSize: 50
    }

    Item {
      width: 1
      height: 1
      Layout.fillHeight: true
    }
  }
}



