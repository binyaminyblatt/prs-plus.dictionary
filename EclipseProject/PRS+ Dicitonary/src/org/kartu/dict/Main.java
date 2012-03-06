package org.kartu.dict;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.charset.Charset;

import org.apache.log4j.Logger;
import org.kartu.IOUtils;
import org.kartu.dict.xdxf.visual.XDXFParser;

import ds.tree.DuplicateKeyException;
import ds.tree.RadixTreeImpl;

public class Main {
	private static final Logger log = Logger.getLogger(Main.class);
	
	public static void main(String[] args) throws IOException, DictionaryParserException {
		int HEADER_SIZE = 1024;
		Charset KEY_CHARSET = Charset.forName("UTF-16LE");
		Charset CHARSET = Charset.forName("UTF-8");
		String inputFileName = "mini.xdxf";
		String outputFileName = "mini.prspdict";

		// Write header (zeros)
		RandomAccessFile raf = new RandomAccessFile(outputFileName, "rw");
		raf.setLength(HEADER_SIZE);
		raf.seek(HEADER_SIZE);

		// Open input xdxf file
		IDictionaryParser parser = new XDXFParser();
		parser.open(inputFileName);
		
		
		RadixTreeImpl<Integer> tree = new RadixTreeImpl<Integer>();
		IDictionaryArticle article;
		int offset = HEADER_SIZE;
		long nArticles = 0;
		while ((article = parser.getNext()) != null) {
			// translation
			byte[] content = article.getTranslation().getBytes(CHARSET);
			try {
				tree.insert(article.getKeyword(), offset);
				IOUtils.writeInt(raf, content.length);
				raf.write(content);
				offset += content.length + 4;
				nArticles++;
			} catch (DuplicateKeyException e) {
				log.warn("Duplicate article: " + article.getKeyword());
			}
		}
		parser.close();
		log.info("Finished reading articles (" + nArticles + ")");

		//------------------------------- Write header --------------------------------------
		log.info("Writing header");
		raf.seek(0);
		// magic
		raf.write("PRSPDICT".getBytes("ASCII"));
		// header size
		IOUtils.writeShort(raf, HEADER_SIZE - 8 /* magic */);
		// version
		raf.write(0); // lo
		raf.write(1); // hi
		// index offset
		IOUtils.writeInt(raf, offset);
		// rewind
		raf.seek(offset);
		
		// Write index
		log.info("Writing indices");
		RadixSerializer.getInstance().persistRadix(KEY_CHARSET, offset, tree.root, raf);
		raf.close();
		
		log.info("OK");
	}
}