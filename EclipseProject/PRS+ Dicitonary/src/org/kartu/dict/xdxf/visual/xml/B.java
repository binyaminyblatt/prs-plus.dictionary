package org.kartu.dict.xdxf.visual.xml;

import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlValue;

/**
 * Corresponds to "b" tag (bold) in xdxf visual format
 * 
 * @author kartu
 */
@XmlRootElement (name="b")
public class B {
	@XmlValue
	public String value;
	
	@Override
	public String toString() {
		return value;
	}
}
