package org.kartu.dict.xdxf.xml;

import java.util.List;

import javax.xml.bind.annotation.XmlAnyElement;
import javax.xml.bind.annotation.XmlMixed;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement (name="c")
public class C {
	@XmlAnyElement (lax=true) @XmlMixed
	public List<Object> elements;
	
	@Override
	public String toString() {
		if (this.elements == null) {
			return null;
		}
		StringBuffer sb = new StringBuffer();
		for (Object o : elements) {
			sb.append(o);
		}
		return sb.toString() + "\n";
	}
}
