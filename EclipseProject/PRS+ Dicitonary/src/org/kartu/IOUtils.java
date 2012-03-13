package org.kartu;

import java.io.IOException;
import java.io.OutputStream;
import java.io.RandomAccessFile;

public class IOUtils {
	// Buffer used when copying from file to file
	private static final int BUF_SIZE = 64 * 1024;

	// Writes big endian short
	public static final int writeShort(RandomAccessFile out, int n) throws IOException {
		out.write(n&0xff);
		out.write((n&0xff00) >>> 8);
		return 2;
	}

	// Writes big endian int
	public static final int writeInt(RandomAccessFile out, int n) throws IOException {
		out.write(n & 0xff);
		out.write((n&0xff00) >> 8);
		out.write((n&0xff0000) >> 16);
		out.write((n&0xff000000) >>> 24);
		return 4;
	}

	// Writes big endian short
	public static final int writeShort(OutputStream out, int n) throws IOException {
		out.write(n&0xff);
		out.write((n&0xff00) >>> 8);
		return 2;
	}

	// Writes big endian int
	public static final int writeInt(OutputStream out, int n) throws IOException {
		out.write(n & 0xff);
		out.write((n&0xff00) >> 8);
		out.write((n&0xff0000) >> 16);
		out.write((n&0xff000000) >>> 24);
		return 4;
	}

	// Writes content of 
	public static void writeRaf(RandomAccessFile raf, RandomAccessFile rafInput) throws IOException {
		long len = rafInput.length();
		byte[] buf = new byte[BUF_SIZE];
		for (long i = 0; i < len - BUF_SIZE; i += BUF_SIZE) {
			rafInput.read(buf);
			raf.write(buf);
		}
		int rest = (int) len % BUF_SIZE;
		rafInput.read(buf, 0, rest);
		raf.write(buf, 0, rest);
	}
}