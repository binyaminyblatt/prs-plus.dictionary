package org.kartu.dict.xdxf.xml;

import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlValue;

@XmlRootElement (name="b")
public class B {
	@XmlValue
	public String value;
	
	@Override
	public String toString() {
		return value;
	}
}
