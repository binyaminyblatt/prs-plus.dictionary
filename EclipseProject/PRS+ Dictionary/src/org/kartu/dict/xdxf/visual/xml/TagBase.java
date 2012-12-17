package org.kartu.dict.xdxf.visual.xml;

import java.util.List;

import javax.xml.bind.annotation.XmlAnyElement;
import javax.xml.bind.annotation.XmlMixed;

public class TagBase {
	@XmlAnyElement (lax=true) @XmlMixed
	public List<Object> elements;
	
	
	@Override
	public String toString() {
		StringBuffer result = new StringBuffer();
		if (this.elements != null) {
			for (Object element : this.elements) {
				if (element != null) result.append(element);
			}
		}
		return result.toString();
	}
}
