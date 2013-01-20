package org.kartu.dict;

/**
 * Holds combination of keyword and corresponding translation.
 * 
 * @author kartu
 */
public interface IDictionaryArticle {
	String getKeyword();
	String getTranslation();
	String getShortTranslation();
	void append(IDictionaryArticle article);
}
