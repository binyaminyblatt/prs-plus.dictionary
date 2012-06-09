package org.kartu.dict.xdxf.visual.xml;

import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlValue;

/**
 * Corresponds to kref (key reference, i.e. "see also") tag in xdxf visual format.
 * 
 * @author kartu
 */
@XmlRootElement (name="kref")
public class KREF {
	@XmlValue
	public String value;
	
	@Override
	public String toString() {
		// "hotlink" ref + value + offset
		return ">>>kref:" + value + "#xxxxxxxx<<<";
	}
}
