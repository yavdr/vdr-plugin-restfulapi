import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.net.UnknownHostException;

public class HTTPRequest {

	/**
	 * @param args
	 * @throws IOException 
	 * @throws UnknownHostException 
	 */
	public static void main(String[] args) throws UnknownHostException, IOException {
				
		String method = "GET";
		String page = "/";
		String body = "";
		
		if ( args.length > 0 ) { method = args[0]; }
		if ( args.length > 1 ) { page = args[1]; }
		if ( args.length > 2 ) { body = args[2]; }
		
		long start = System.currentTimeMillis();
		
		Socket socket = new Socket("127.0.0.1", 8002);
		
		BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
		BufferedWriter bufferedWriter = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
		
		bufferedWriter.write(method + " " + page + " HTTP/1.1");
		bufferedWriter.newLine();
		bufferedWriter.write("Content-Length: "+body.length());
		bufferedWriter.newLine();
		bufferedWriter.write("Connection: close");
		bufferedWriter.newLine();
		bufferedWriter.newLine();
		bufferedWriter.write(body);
		bufferedWriter.newLine();
		bufferedWriter.flush();
		
		String line = null;
		while((line = bufferedReader.readLine()) != null) {
			System.out.println(line);
		}
		
		bufferedReader.close();
		bufferedWriter.close();
		socket.close();
		
		long stop = System.currentTimeMillis();
		System.out.println("Time: "+(stop-start));
	}

}
