package dk.aau.group1.p5.chess.http;

import org.junit.jupiter.api.Test;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.AutoConfigureMockMvc;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.http.MediaType;
import org.springframework.test.context.bean.override.mockito.MockitoBean;
import org.springframework.test.web.servlet.MockMvc;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.*;
import com.fasterxml.jackson.databind.ObjectMapper;

import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.*;
import static org.springframework.test.web.servlet.result.MockMvcResultHandlers.print;


import dk.aau.group1.p5.chess.model.User;
import dk.aau.group1.p5.chess.web.UserController;

@AutoConfigureMockMvc
@WebMvcTest(UserController.class)
class HttpUserControllerTest {

    @Autowired
    private ObjectMapper objectMapper;

    @Autowired
    private MockMvc mvc;

    @MockitoBean
    private UserController controller;

    @Test
    void connectionTest() throws Exception {

        mvc.perform(get("/users")).andExpect(status().isOk());
    }

	@Test
	void addUserTest() throws Exception {
        User u = new User();
        u.setName("test");
        //u.setId((long)(1));
        var action = mvc.perform(post("/users").contentType(MediaType.APPLICATION_JSON).content(objectMapper.writeValueAsString(u)));
        
        action.andDo(print());
        action.andExpect(status().isOk());


        var out = action.andReturn().getResponse().getContentAsString();
        u.setName(out);
        //action.andExpect(jsonPath("$.data.id").value("name"));
        //action.andExpect(jsonPath("id").value(1));
        
        
    
    }

}
