package org.kartu.dict.xdxf.xml;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Unmarshaller;
import javax.xml.bind.ValidationEvent;
import javax.xml.bind.ValidationEventHandler;
import javax.xml.bind.annotation.XmlAnyElement;
import javax.xml.bind.annotation.XmlMixed;
import javax.xml.bind.annotation.XmlRootElement;

import org.apache.commons.lang.builder.ToStringBuilder;

@XmlRootElement (name="ar")
public class AR {
	@XmlAnyElement (lax=true) @XmlMixed
	public List<Object> elements;
	
	public static Unmarshaller unmarshaller;
	static {
		try {
			JAXBContext ctx = JAXBContext.newInstance(AR.class, B.class, C.class, I.class, K.class, TR.class);
			unmarshaller = ctx.createUnmarshaller();
			unmarshaller.setEventHandler(new ValidationEventHandler() {
				public boolean handleEvent(ValidationEvent event) {
					System.out.println(event);
					// TODO Auto-generated method stub
					return false;
				}
				
			});
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	}
	
	public static void main(String[] args) throws JAXBException {
		unmarshaller.setEventHandler(new ValidationEventHandler() {
			public boolean handleEvent(ValidationEvent event) {
				System.out.println(event);
				return false;
			}
			
		});
		AR a = (AR) unmarshaller.unmarshal(new File("ar.xml"));
		System.out.println(ToStringBuilder.reflectionToString(a));
	}

	public List<String> getKeywords() {
		ArrayList<String> result = new ArrayList<String>();
		for (Object o : this.elements) {
			if (o instanceof K) {
				result.add(((K)o).value);
			}
		}
		
		return result;
	}
	
	public String toString() {
		StringBuffer result = new StringBuffer();
		result.append("");
		for (Object element : this.elements) {
			result.append(element);
		}
		result.append("\n");
		return result.toString();
	}

	public String getTranslation() {
		return this.toString();
	}
}
