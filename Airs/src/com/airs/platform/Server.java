package com.airs.platform;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class Server {

	private int m_Port;
	private EventComponent m_EventComponent;
	private Acquisition m_Acquisition;
	private final byte[] m_From = "REMONT AS".getBytes();
	private final byte[] m_EventName = "acquire".getBytes();


	public Server() {
		this(9000);
	}
	
	public Server(int port) {
		this.m_Port = port;
	}
	
	public Server(int port, EventComponent eventComponent, Acquisition acquisition) {
		this.m_Port = port;
		this.m_EventComponent = eventComponent;
		this.m_Acquisition = acquisition;
		
	}

	public void startConnection() throws IOException {
		ServerSocket serverSock = new ServerSocket(this.m_Port);
		// wait for a connection.
		Socket sock = serverSock.accept();
		
		if (this.m_EventComponent == null)
			this.m_EventComponent = new EventComponent();

		// For reading sensor values.
		if (this.m_Acquisition == null)
			this.m_Acquisition = new Acquisition(this.m_EventComponent);
		
		// For reading sensor descriptions.
		new Discovery(this.m_EventComponent);

		m_EventComponent.startEC(sock);
	}
	
	public void subscribe(String sensorCode) {
		
		byte[] sensor = sensorCode.getBytes();
		int expires = 3600;
		
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