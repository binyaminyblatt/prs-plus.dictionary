package org.kartu.dict.stardict;

import org.kartu.dict.IDictionaryArticle;

public class Article implements IDictionaryArticle {
	private String shortTranslation;
	private String translation;
	private String keyword;
	
	public Article(String keyword, String translation, String shortTranslation) {
		this.keyword = keyword;
		this.translation = translation;
		this.shortTranslation = shortTranslation;
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
		return shortTranslation;
	}

}
