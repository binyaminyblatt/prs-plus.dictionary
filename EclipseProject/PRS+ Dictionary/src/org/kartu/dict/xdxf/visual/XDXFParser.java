package org.kartu.dict.xdxf.visual;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.List;

import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamReader;

import org.kartu.dict.DictionaryParserException;
import org.kartu.dict.IDictionaryArticle;
import org.kartu.dict.IDictionaryParser;
import org.kartu.dict.xdxf.visual.xml.AR;

/**
 * Serial dictionary parser of XDXF visual format.
 * 
 * @author kartu
 * 
 * @see IDictionaryParser
 */
public class XDXFParser implements IDictionaryParser, IDictionaryArticle {
	private XMLStreamReader xmlStreamReader;
	private FileInputStream fin;
	private AR ar;
	
	@Override
	public void open(String path) throws IOException, DictionaryParserException {
		// StaX
		XMLInputFactory f = XMLInputFactory.newInstance();
		f.setProperty("javax.xml.stream.supportDTD", false);
		f.setProperty("javax.xml.stream.isSupportingExternalEntities", false);
		this.fin = new FileInputStream(path);
		try {
			this.xmlStreamReader = f.createXMLStreamReader(this.fin);
		} catch (XMLStreamException e) {
			throw new DictionaryParserException(e);
		}
	}

	@Override
	public IDictionaryArticle getNext() throws DictionaryParserException {
		try {
			// Read article by article, ignoring other tags
			while (this.xmlStreamReader.hasNext()) {
				this.xmlStreamReader.next();
				if (this.xmlStreamReader.isStartElement()) {
			    	String tagName = this.xmlStreamReader.getLocalName();
			    	if ("ar".equals(tagName)) {
						AR ar = (AR) AR.unmarshaller.unmarshal(this.xmlStreamReader);
						this.ar = ar;
						return this;
			    	}
				}
			}
		} catch (Exception e) {
			throw new DictionaryParserException(e);
		}
		return null;
	}

	@Override
	public void close() {
		try {
			this.xmlStreamReader.close();
			this.fin.close();
		} catch (Exception ignore) {
		}
	}

	@Override
	public String getKeyword() {
		List<String> keywords = this.ar.getKeywords();
		return keywords.size() > 0 ? (keywords.get(0)).trim() : null;
	}

	@Override
	public String getTranslation() {
		return this.ar.getTranslation();
	}

	@Override
	public String getShortTranslation() {
		return this.ar.getShortTranslation();
	}

}
