package org.kartu.dict.xdxf.visual.xml;

import java.util.List;

import javax.xml.bind.annotation.XmlAnyElement;
import javax.xml.bind.annotation.XmlMixed;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Corresponds to "nu" tag (key phrase) in xdxf visual format
 * 
 * @author kartu
 */

@XmlRootElement (name="nu")
public class NU {
	@XmlAnyElement (lax=true) @XmlMixed
	public List<Object> elements;
	
	@Override
	public String toString() {
		if (this.elements == null) {
			return "";
		}
		StringBuffer sb = new StringBuffer();
		for (Object o : elements) {
			if (o != null) sb.append(o);
		}
		return sb.toString();
	}
}
