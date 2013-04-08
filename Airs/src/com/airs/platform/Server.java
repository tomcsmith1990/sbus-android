package com.airs.platform;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class Server {

	private int m_Port;
	private EventComponent m_EventComponent;
	private Acquisition m_Acquisition;
	private Discovery m_Discovery;
	private final byte[] m_From = "REMONT AS".getBytes();
	private final byte[] m_EventName = "acquire".getBytes();
	private ServerSocket m_ServerSock;

	public Server() {
		this(9000);
	}

	public Server(int port) {
		this.m_Port = port;
	}

	public Server(int port, EventComponent eventComponent, Acquisition acquisition, Discovery discovery) {
		this.m_Port = port;
		this.m_EventComponent = eventComponent;
		this.m_Acquisition = acquisition;
		this.m_Discovery = discovery;
	}

	public void startConnection() throws IOException {
		m_ServerSock = new ServerSocket(this.m_Port);
		// wait for a connection.
		Socket sock = m_ServerSock.accept();

		if (this.m_EventComponent == null)
			this.m_EventComponent = new EventComponent();

		// For reading sensor values.
		if (this.m_Acquisition == null)
			this.m_Acquisition = new Acquisition(this.m_EventComponent);

		// For reading sensor descriptions.
		if (this.m_Discovery == null)
			this.m_Discovery = new Discovery(this.m_EventComponent);


		m_EventComponent.startEC(sock);
	}

	public void stop() throws IOException {
		m_ServerSock.close();
		if (m_EventComponent != null)
			m_EventComponent.stop();
	}

	public void subscribe(String sensorCode) {

		if (this.m_EventComponent == null) return;

		byte[] sensor = sensorCode.getBytes();
		int expires = 0;

		this.m_EventComponent.Subscribe(this.m_From, this.m_EventName, sensor, sensor.length, expires, this.m_Acquisition);
	}

	public static void main(String[] args) throws IOException {
		Server server = new Server();
		server.startConnection();
		// Random number
		server.subscribe("Rd");
		// WLAN SSID
		server.subscribe("WI");
	}
}
