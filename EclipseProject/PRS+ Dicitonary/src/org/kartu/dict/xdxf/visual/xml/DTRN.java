package org.kartu.dict.xdxf.visual.xml;

import java.util.List;

import javax.xml.bind.annotation.XmlAnyElement;
import javax.xml.bind.annotation.XmlMixed;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Corresponds to dtrn (direct translation) tag in xdxf visual format
 * 
 * @author kartu
 */
@XmlRootElement (name="dtrn")
public class DTRN {
	@XmlAnyElement (lax=true) @XmlMixed
	public List<Object> elements;
	
	@Override
	public String toString() {
		StringBuffer result = new StringBuffer();
		result.append("{");
		for (Object element : this.elements) {
			if (element != null) result.append(element);
		}
		result.append("}");
		return result.toString();
	}
}
