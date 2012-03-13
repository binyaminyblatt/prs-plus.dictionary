package org.kartu.dict;

import java.io.File;
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
	static final int HEADER_SIZE = 1024;
	static final Charset KEY_CHARSET = Charset.forName("UTF-16LE");
	static final Charset ARTICLE_CHARSET = Charset.forName("UTF-8");
	private static final int SHORT_TRANSLATION_LEN = 80;
	
	public static void main(String[] args) throws IOException, DictionaryParserException {
		if (args.length != 2) {
			printUsage();
			System.exit(0);
		}
		
		String inputFileName = args[0];
		String outputFileName = args[1];

		// Write header (zeros)
		RandomAccessFile raf = new RandomAccessFile(outputFileName, "rw");
		raf.setLength(HEADER_SIZE);
		raf.seek(HEADER_SIZE);
		
		// Temporary file for quick lookup of words with closes match
		RandomAccessFile rafTemp = new RandomAccessFile(File.createTempFile("prspDictTemp", ".prspdict"), "rw");

		// Open input xdxf file
		IDictionaryParser parser = new XDXFParser();
		parser.open(inputFileName);
		
		
		RadixTreeImpl<int[]> tree = new RadixTreeImpl<int[]>();
		IDictionaryArticle article;
		int offset = 0;
		int shortOffset = 0;
		long nArticles = 0;
		while ((article = parser.getNext()) != null) {
			// translation
			byte[] content = article.getTranslation().getBytes(ARTICLE_CHARSET);
			// keyword + short translation
			String shortTranslation = article.getKeyword() + '\0' + article.getTranslation().substring(0, SHORT_TRANSLATION_LEN) + '\0';
			byte[] shortContent = shortTranslation.getBytes(ARTICLE_CHARSET);
			try {
				tree.insert(article.getKeyword(), new int[] {offset, shortOffset});
				IOUtils.writeInt(raf, content.length);
				raf.write(content);
				rafTemp.write(shortContent);
				offset += content.length + 8;
				shortOffset += shortContent.length;
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
		IOUtils.writeInt(raf, offset + shortOffset + HEADER_SIZE);
		
		// rewind
		raf.seek(offset + HEADER_SIZE);
		
		// write word list
		rafTemp.seek(0);
		IOUtils.writeRaf(raf, rafTemp);
		
		// Write index
		log.info("Writing indices");
		RadixSerializer.getInstance().persistRadix(KEY_CHARSET, offset + shortOffset + HEADER_SIZE, HEADER_SIZE, HEADER_SIZE + offset, tree.root, raf);
		raf.close();
		
		log.info("OK");
	}

	private static void printUsage() {
		System.out.println("Usage:\n\t... <input xdxf file> <output file>");
	}
}