package org.kartu.dict.xdxf.visual.xml;

import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlValue;

/**
 * Corresponds to "i" tag (italic) in xdxf visual format 
 * 
 * @author kartu
 */
@XmlRootElement (name="i")
public class I {
	@XmlValue
	public String value;
	
	@Override
	public String toString() {
		return value;
	}
}
