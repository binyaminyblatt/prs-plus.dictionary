package org.kartu.dict.xdxf.visual;

import java.util.List;

import org.kartu.dict.Article;
import org.kartu.dict.xdxf.visual.xml.AR;

public class XDXFArticle extends Article {
	public XDXFArticle(AR ar) {
		List<String> keywords = ar.getKeywords();
		this.keyword =  keywords.size() > 0 ? (keywords.get(0)).trim() : null;
		this.translation = ar.getTranslation();
		this.shortTranslation = ar.getShortTranslation();
	}
}
