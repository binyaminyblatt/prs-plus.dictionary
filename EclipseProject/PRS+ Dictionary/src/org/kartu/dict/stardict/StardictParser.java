package org.kartu.dict.stardict;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.util.HashMap;
import java.util.Map;
import java.util.zip.GZIPInputStream;

import org.apache.log4j.Logger;
import org.kartu.dict.DictionaryParserException;
import org.kartu.dict.IDictionaryArticle;
import org.kartu.dict.IDictionaryParser;
import org.kartu.dict.xdxf.visual.xml.AR;

public class StardictParser implements IDictionaryParser, IDictionaryArticle {
	static final Logger log = Logger.getLogger(StardictParser.class);
	public static final String EXTENSION = ".ifo";
	private Map<String, String> ifoMap;
	private ArticleType articleType;
	private byte[] dictData;
	private byte[] idxData;
	private int cursor;
	private String keyword;
	private String translation;

	enum ArticleType {XDXF, TEXT};
	
	byte[] readFile(String path) throws IOException {
		File f = new File(path);
		if (!f.isFile()) {
			fatal("Internal error: File "  + path + " doesn't exist!");
			return null;
		}
		
		int size = (int) f.length();
		byte[] buf = new byte[size];
		FileInputStream fin = new FileInputStream(f);
		fin.read(buf);

		fin.close();
		
		return buf;
	}
	
	@Override
	public void open(String ifoPath) throws IOException, DictionaryParserException {
		// Parse ifo file
		ifoMap = parseIFO(ifoPath);
		
		// Check if we can  parse it at all
		if (ifoMap.get("idxoffsetbits") != null && !"32".equals("idxoffsetbits")) {
			fatal("idxoffsetbits of length 64 is not supported!");
		}
		
		String sequenceType = ifoMap.get("sametypesequence");
		if ("x".equals(sequenceType)) {
			this.articleType = ArticleType.XDXF;
		} else if ("t".equals(sequenceType)) {
			this.articleType = ArticleType.TEXT;
		} else {
			fatal("Unsuported article type:"  + sequenceType);
		}

		
		// Ensure all needed files exist
		int idx = ifoPath.lastIndexOf('.');
		String idxPath, dictPath, dictGzPath;
		String tmp;
		if (idx > 0) {
			tmp = ifoPath.substring(0, idx);
		} else {
			tmp = ifoPath;
		}
		
		idxPath = tmp + ".idx";
		dictPath = tmp + ".dict";
		dictGzPath = dictPath + ".dz";
			
		if (! new File(idxPath).exists()) {
			fatal("Cannot file .idx file: " + idxPath);
		}

		InputStream dzIn = null;
		
		if (new File(dictPath).exists()) {
			dzIn = new FileInputStream(dictPath);
		} else if (new File(dictGzPath).exists()) {
			dzIn = new GZIPInputStream(new FileInputStream(dictGzPath));
		}
		if (! new File(dictPath).exists() && ! new File(dictGzPath).exists()) {
			fatal("Cannot find neither .dict nor dict.dz file: " + dictPath);
		}
		
		// Read dz file into memory
		ByteArrayOutputStream bout = new ByteArrayOutputStream();
		byte[] buf = new byte[1024];
		int n;
		while ((n  = dzIn.read(buf)) > 0) {
			bout.write(buf, 0, n);
		}
		this.dictData = bout.toByteArray();
		
		dzIn.close();
		
		// Read index file
		this.idxData = readFile(idxPath);
		this.cursor = 0;
	}

	private Map<String, String> parseIFO(String ifoPath) throws IOException {
		String[] lines = new String(readFile(ifoPath)).split("\n");
		Map<String, String> result = new HashMap<String, String>(8);
		for (String line : lines) {
			if (line.contains("=")) {
				int idx = line.indexOf('=');
				String key = line.substring(0, idx);
				String value = line.substring(idx + 1);
				log.info(String.format("[%s] => [%s]", key, value));
				result.put(key, value);
			}
		}
		
		return result;
	}

	private void fatal(String msg) {
		System.out.println(msg);
		System.exit(1);
	}

	@Override
	public IDictionaryArticle getNext() throws DictionaryParserException {
		if (idxData.length > cursor) {
			for (int n = cursor; n < idxData.length; n++) {
				if (idxData[n] == 0) {
					try {
						String keyword = new String(idxData, cursor, n - cursor, "UTF-8");
						this.keyword = keyword;
						// article offset
						int offset = 256*256*256 * (idxData[n+1] & 0xff) +
									256*256 * (idxData[n+2] & 0xff) +
									256 * (idxData[n+3] & 0xff)
									+ (idxData[n+4] & 0xff);
						// article length
						int len = 256*256*256 * (idxData[n+5] & 0xff) +
								256*256 * (idxData[n+6] & 0xff) +
								256 * (idxData[n+7] & 0xff)
								+ (idxData[n+8] & 0xff);

						this.translation = new String(dictData, offset, len);
						if (articleType == ArticleType.XDXF) {
							// Convert XDXF
							this.translation = "<ar>" + this.translation + "</ar>";
							this.translation = AR.unmarshaller.unmarshal(
									new StringReader(this.translation)).toString();
						}
						
						cursor = n + 8 + 1;
						return this;
					} catch (Exception e) {
						throw new RuntimeException("[" + this.translation + "]", e);
					}
				}
			}
		}
		return null;
	}

	@Override
	public void close() {
	}

	@Override
	public String getKeyword() {
		return keyword;
	}

	@Override
	public String getTranslation() {
		return translation;
	}

	@Override
	public String getShortTranslation() {
		return translation;
	}

}
