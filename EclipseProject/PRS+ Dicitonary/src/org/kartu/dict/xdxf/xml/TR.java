package org.kartu.dict.xdxf.xml;

import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlValue;

@XmlRootElement (name="tr")
public class TR {
	@XmlValue
	public String value;
	
	@Override
	public String toString() {
		return " [" + value + "]\n";
	}
}
