package org.kartu.dict.xdxf.visual.xml;

import java.util.List;

import javax.xml.bind.annotation.XmlAnyElement;
import javax.xml.bind.annotation.XmlMixed;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Corresponds to "i" tag (italic) in xdxf visual format 
 * 
 * @author kartu
 */
@XmlRootElement (name="i")
public class I {
	@XmlAnyElement (lax=true) @XmlMixed
	public List<Object> elements;
	
	@Override
	public String toString() {
		StringBuffer result = new StringBuffer();
		if (this.elements != null) {
			result.append("_");
			for (Object element : this.elements) {
				if (element != null) result.append(element);
			}
			result.append("_");
		}
		return result.toString();
	}
}
