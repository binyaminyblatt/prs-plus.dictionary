package org.kartu.dict.xdxf.visual.xml;

import java.util.List;

import javax.xml.bind.annotation.XmlAnyElement;
import javax.xml.bind.annotation.XmlMixed;
import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlValue;

/**
 * Corresponds to "b" tag (bold) in xdxf visual format
 * 
 * @author kartu
 */
@XmlRootElement (name="b")
public class B {
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
		return "*" + sb.toString() + "*";
	}
}
