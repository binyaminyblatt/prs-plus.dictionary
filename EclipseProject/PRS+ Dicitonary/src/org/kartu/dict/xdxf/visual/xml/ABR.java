package org.kartu.dict.xdxf.visual.xml;

import java.util.List;

import javax.xml.bind.annotation.XmlAnyElement;
import javax.xml.bind.annotation.XmlMixed;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Corresponds to abr (abbreviation) tag in xdxf visual format
 * 
 * @author kartu
 */
@XmlRootElement (name="abr")
public class ABR {
	@XmlAnyElement (lax=true) @XmlMixed
	public List<Object> elements;
	
	@Override
	public String toString() {
		StringBuffer result = new StringBuffer();
		for (Object element : this.elements) {
			result.append(element);
		}
		return result.toString();
	}

}
