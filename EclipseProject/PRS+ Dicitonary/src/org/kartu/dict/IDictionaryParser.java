package org.kartu.dict;

import java.io.IOException;

public interface IDictionaryParser {
	/**
	 * Opens XDXF dictionary file for reading
	 * 
	 * @param path
	 * @throws IOException
	 * @throws DictionaryParserException
	 */
	void open(String path) throws IOException, DictionaryParserException;
	
	/**
	 * 
	 * @return next article or null, if end of stream was reached.
	 * @throws DictionaryParserException 
	 */
	IDictionaryArticle getNext() throws DictionaryParserException;
	
	/**
	 * Closes IO streams opened  by {@link #open(String)}
	 */
	void close();
}
