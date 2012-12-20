package org.kartu.dict.xdxf.visual.xml;

import javax.xml.bind.annotation.XmlRootElement;

/**
 * Corresponds to "k" tag (keyword) in xdxf visual format
 * 
 * @author kartu
 */
@XmlRootElement (name="k")
public class K extends TagBase {
	@Override
	public String toString() {
		return ">>>" + super.toString() + "<<<";
	}
}

