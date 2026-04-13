package dk.aau.group1.p5.chess.web;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import dk.aau.group1.p5.chess.model.User;

@SpringBootTest
public class UserControllerTest {

    @Autowired
    private UserController c;

    //public UserControllerTest(UserService userService) {
    //    c = new UserController(userService);
    //}

    @Test
    void testAddUser() {
        final String name = "theis";
        User u = new User();
        u.setName(name);
        assertNull(u.getId());

        User u2 = c.addUser(u);
        assertNotNull(u2.getId());
        assertEquals(u2.getName(), name);
    }

    @Test
    void testGetAllUsers() {
        var l = c.getAllUsers();
        assertFalse(l.size() < 1);
    }

    @Test
    void testUpdateUser() {

    }
}
