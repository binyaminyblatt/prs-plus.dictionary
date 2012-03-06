package org.kartu;

import java.io.IOException;
import java.io.OutputStream;
import java.io.RandomAccessFile;

public class IOUtils {

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
	
}
