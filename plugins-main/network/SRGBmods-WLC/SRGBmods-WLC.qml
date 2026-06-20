Item {
	anchors.fill: parent
	Column {
		width: parent.width
		height: parent.height
		Column {
			width: 450
			height: 105
			Rectangle {
				width: parent.width
				height: parent.height - 10
				color: "#333bff"
				radius: 5
				Column {
					x: 10
					y: 10
					width: parent.width - 20
					spacing: 0
					Text {
						color: theme.primarytextcolor
						textFormat: Text.RichText
						text: "<table><tr><td width=\"24\" style=\"text-align:center;vertical-align:middle\"><img src=\"https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/info-circle-fill.png\"></style></td><td><u><strong>Important:<strong></u><br>this service <strong>ONLY</strong> works for the DIY (do-it-yourself) micro controllers<br><strong>Raspberry Pi Pico W</strong> and <strong>Arduino Nano RP2040 Connect</strong><br><strong>flashed with <style>a:link { color: \"#FFFFFF\"; }</style><a href=\"https://srgbmods.net/wifilc\">SRGBmods Wifi LC firmware</a></strong>!</td></tr></table>"
						onLinkActivated: Qt.openUrlExternally(link)
						font.pixelSize: 12
						font.family: "Poppins"
						font.bold: false
					}
				}
			}
		}
		Column {
			width: 450
			height: 115
			Rectangle {
				width: parent.width
				height: parent.height - 10
				color: "#141414"
				radius: 5
				Column {
					x: 10
					y: 10
					width: parent.width - 20
					spacing: 0
					Text {
						color: theme.primarytextcolor
						text: "Discover WLC device by IP"
						font.pixelSize: 16
						font.family: "Poppins"
						font.bold: true
					}
					Row {
						spacing: 6
						Image {
							x: 10
							y: 10
							height: 40				
							source: "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/wlc-service.png"
							fillMode: Image.PreserveAspectFit
							antialiasing: true
							mipmap: true
						}
						Rectangle {
							x: 10
							y: 6
							width: 200
							height: 50
							radius: 5
							border.color: "#1c1c1c"
							border.width: 2
							color: "#141414"
							TextField {
								width: 180
								leftPadding: 10
								rightPadding: 10
								id: discoverIP
								x: 10
								color: theme.primarytextcolor
								font.family: "Poppins"
								font.bold: true
								font.pixelSize: 20
								verticalAlignment: TextInput.AlignVCenter
								placeholderText: "192.168.0.1"
								onEditingFinished: {
									discovery.forceDiscover(discoverIP.text);
								}
								validator: RegularExpressionValidator {
									regularExpression:  /^((?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\.){0,3}(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$/
								}
								background: Item {
									width: parent.width
									height: parent.height
									Rectangle {
										color: "transparent"
										height: 1
										width: parent.width
										anchors.bottom: parent.bottom
									}
								}
							}
						}
					}
				}
				Column {
					x: 260
					y: 4
					width: parent.width - 20
					spacing: 10
					Image {
						x: 20
						y: 10
						height: 40				
						source: "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/wlc-logo_mono.png"
						fillMode: Image.PreserveAspectFit
						antialiasing: true
						mipmap: true
					}
				}
				Column {
					x: 285
					y: 60
					width: parent.width - 20
					spacing: 10
					Item{
						Rectangle {
							width: 120
							height: 26
							color: "#D65A00"
							radius: 5
						}
						width: 120
						height: 26
						ToolButton {
							height: 30
							width: 120
							anchors.verticalCenter: parent.verticalCenter
							font.family: "Poppins"
							font.bold: true
							icon.source: "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/icon-discover.png"
							text: "Discover"
							anchors.right: parent.right
							onClicked: {
								discovery.forceDiscover(discoverIP.text);
							}
						}
					}
				}
			}
		}

		ListView {
			id: controllerList
			model: service.controllers   
			width: contentItem.childrenRect.width + (controllerListScrollBar.width * 1.5)
			height: parent.height - 265
			clip: true

			ScrollBar.vertical: ScrollBar {
				id: controllerListScrollBar
				anchors.right: parent.right
				width: 10
				visible: parent.height < parent.contentHeight
				policy: ScrollBar.AlwaysOn

				height: parent.availableHeight
				contentItem: Rectangle {
					radius: parent.width / 2
					color: theme.scrollBar
				}
			}


			delegate: Item {
				visible: true
				width: 450
				height: 115
				property var device: model.modelData.obj

				Rectangle {
					width: parent.width
					height: parent.height - 10
					color: device.offline ? "#101010" : device.connected ? "#8f155f" : "#292929"
					radius: 5
					
					Image {
					   anchors.fill: parent
					   source: device.offline ? "" : device.connected ? "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/bg-linked.png" : ""
					}
				}
				Column {
					x: 260
					y: 4
					width: parent.width - 20
					spacing: 10
					Image {
						x: 20
						y: 10
						height: 40				
						source: "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/wlc-logo_mono.png"
						fillMode: Image.PreserveAspectFit
						antialiasing: true
						mipmap: true
					}
				}
				Column {
					x: 285
					y: 60
					width: parent.width - 20
					spacing: 10
					Item{
						Rectangle {
							width: 120
							height: 26
							color: device.offline ? "#C0A21B" : device.connected ? "#292929" : "#8f155f"
							radius: 5
							
							Image {
							   anchors.fill: parent
							   source: device.offline ? "" : !device.connected ? "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/device-link.png" : ""
							}
							MouseArea {
								anchors.fill: parent
								acceptedButtons: Qt.NoButton
								cursorShape: Qt.ForbiddenCursor
							}
						}
						width: 120
						height: 26
						ToolButton {
							height: 30
							width: 120
							anchors.verticalCenter: parent.verticalCenter
							font.family: "Poppins"
							font.bold: true 
							visible: device.offline ? false : device.connected
							text: "Unlink"
							anchors.right: parent.right
							onClicked: {
								device.startRemove();
							}
						}
						ToolButton {
							height: 30
							width: 120
							anchors.verticalCenter: parent.verticalCenter
							font.family: "Poppins"
							font.bold: true 
							visible: device.offline ? false : !device.connected
							text: "Link"
							anchors.right: parent.right
							onClicked: {
								device.startLink();
							}
						}
						Text {
							anchors.verticalCenter: parent.verticalCenter
							anchors.horizontalCenter: parent.horizontalCenter
							color: theme.primarytextcolor
							font.pixelSize: 15
							font.family: "Poppins"
							font.bold: true 
							visible: device.offline
							text: "OFFLINE!"
						}
					}
				}
				Column {
					x: 10
					y: 4
					spacing: 6
					Row {
						width: parent.width - 20
						spacing: 6

						Text {
							color: theme.primarytextcolor
							text: device.name
							font.pixelSize: 16
							font.family: "Poppins"
							font.bold: true
						}
						Image {
							y: 3
							id: iconSignalStrength
							source: device.offline ? "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/device-offline.png" : device.signalstrength >= 90 ? "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/device-signal4.png" : device.signalstrength >= 75 ? "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/device-signal3.png" : device.signalstrength >= 60 ? "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/device-signal2.png" : device.signalstrength >= 50 ? "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/device-signal1.png" : "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/device-signal0.png"
						}
					}
					Row {
						spacing: 6
						Image {
							visible: device.offline ? false : true
							id: iconTurnOn
							source: "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/device-turnon.png"
							width: 16; height: 16
							opacity: 1.0
							MouseArea {
								anchors.fill: parent
								hoverEnabled: true
								acceptedButtons: Qt.LeftButton
								onClicked: {
									 device.turnOn();
								}
								onEntered: {
									iconTurnOn.opacity = 0.8;
								}
								onExited: {
									iconTurnOn.opacity = 1.0;
								}
							}
						}
						Image {
							visible: device.offline ? false : true
							id: iconTurnOff
							source: "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/device-turnoff.png"
							width: 16; height: 16
							opacity: 1.0
							MouseArea {
								anchors.fill: parent
								hoverEnabled: true
								acceptedButtons: Qt.LeftButton
								onClicked: {
									 device.turnOff();
								}
								onEntered: {
									iconTurnOff.opacity = 0.8;
								}
								onExited: {
									iconTurnOff.opacity = 1.0;
								}
							}
						}
						Image {
							id: iconDelete
							source: "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/device-delete.png"
							width: 16; height: 16
							visible: device.forced ? true : false
							opacity: 1.0
							MouseArea {
								anchors.fill: parent
								hoverEnabled: true
								acceptedButtons: Qt.LeftButton
								onClicked: {
									 device.startDelete();
								}
								onEntered: {
									iconDelete.opacity = 0.8;
								}
								onExited: {
									iconDelete.opacity = 1.0;
								}
							}
						}
						Image {
							id: iconForceAdd
							source: "https://raw.githubusercontent.com/SRGBmods/public/main/images/wlc/device-forceadd.png"
							width: 16; height: 16
							visible: device.forced ? false : true
							opacity: 1.0
							MouseArea {
								anchors.fill: parent
								hoverEnabled: true
								acceptedButtons: Qt.LeftButton
								onClicked: {
									 device.startForceDiscover();
								}
								onEntered: {
									iconForceAdd.opacity = 0.8;
								}
								onExited: {
									iconForceAdd.opacity = 1.0;
								}
							}
						}
					}
					Text {
						color: theme.primarytextcolor
						text: "MAC: " + device.mac + "  |  IP: " + device.ip + ":" + device.streamingPort
					}
					Text {
						color: theme.primarytextcolor
						text: "MCU: " + device.arch + " @ " + device.firmwareversion + "  |  LED count: " + device.deviceledcount
					}		  
				}
			}
		}
	}
}