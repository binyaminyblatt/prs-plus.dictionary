package org.kartu.dict.xdxf.visual.xml;

import java.util.List;

import javax.xml.bind.annotation.XmlAnyElement;
import javax.xml.bind.annotation.XmlMixed;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Corresponds to ex (example) tag in xdxf visual format
 * 
 * @author kartu
 */
@XmlRootElement (name="ex")
public class EX {
	@XmlAnyElement (lax=true) @XmlMixed
	public List<Object> elements;
	
	@Override
	public String toString() {
		StringBuffer result = new StringBuffer();
		result.append("{");
		for (Object element : this.elements) {
			result.append(element);
		}
		result.append("}");
		return result.toString();
	}
}