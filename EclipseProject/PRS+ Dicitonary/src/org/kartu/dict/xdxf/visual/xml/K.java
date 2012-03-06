package org.kartu.dict.xdxf.visual.xml;

import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlValue;

/**
 * Corresponds to "k" tag (keyword) in xdxf visual format
 * 
 * @author kartu
 */
@XmlRootElement (name="k")
public class K {
	@XmlValue
	public String value;
	
	@Override
	public String toString() {
		return ">>>" + value + "<<<\n";
	}
}
