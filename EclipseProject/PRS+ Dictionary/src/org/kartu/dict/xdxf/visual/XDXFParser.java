package org.kartu.dict.xdxf.visual;

import java.io.FileInputStream;
import java.io.IOException;

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
public class XDXFParser implements IDictionaryParser {
	private XMLStreamReader xmlStreamReader;
	private FileInputStream fin;
	public static final String EXTENSION = ".xdxf";
	
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
						return new XDXFArticle(ar);
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
}
