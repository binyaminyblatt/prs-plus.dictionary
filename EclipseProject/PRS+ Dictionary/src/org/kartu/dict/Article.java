package org.kartu.dict;


public class Article implements IDictionaryArticle {

	protected String shortTranslation;
	protected String translation;
	protected String keyword;

	public Article() {
		super();
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

	@Override
	public void append(IDictionaryArticle article) {
		if (!this.translation.endsWith("\n")) {
			this.translation += "\n";
		}
		this.translation += article.getTranslation();
	}

}