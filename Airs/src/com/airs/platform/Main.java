package com.airs.platform;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class Main {

	public static void main(String[] args) throws IOException {
		ServerSocket serverSock = new ServerSocket(9000);
		EventComponent ec = new EventComponent();
		Socket sock = serverSock.accept();
		byte[] IMEI = new byte[15];
		sock.getInputStream().read(IMEI);
		System.out.println(new String(IMEI));
		
		ec.startEC(sock);
		Acquisition ac = new Acquisition(ec);
		Discovery dc = new Discovery(ec);
		
		byte[] FROM = "REMONT AS".getBytes();

		//ec.Subscribe(FROM, "acquire".getBytes(), "Rd".getBytes(), "Rd".getBytes().length, 3600, ac);
	}
}
